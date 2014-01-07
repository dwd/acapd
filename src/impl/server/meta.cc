/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <errno.h>

#include <string>
#include <iostream>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <fstream>
#include <cstdlib>

#ifndef UNIX_PATH_MAX /* Mentioned by the man pages, but not in the header. */
const unsigned int UNIX_PATH_MAX(108); /* Both header and manpage agree over this value. */
#endif

class Meta {
 public:
  Meta();
  int run( int argc, char * argv[] );
  static void shutdown( int );
  static void restart( int );
  static void reap( int );
  void kill( bool shutdown );
  void log( int prio, std::string const & what );
  
 private:
  int launch();
  void command();
  void handle_command( char c, std::string const & w );
  void do_listen( std::string const & );
  void do_close( std::string const & );
  void open_syslog();
  
 public:
  static Meta * instance;
 private:
  typedef std::map< std::string, int > t_listenfds;
  typedef std::map< std::string, std::string > t_options;
  typedef std::list< std::string > t_args;
  t_listenfds m_listenfds;
  t_options m_options;
  t_args m_args;
  int m_ctrlfd;
  int m_remote_fd;
  uid_t m_acap_uid;
  uid_t m_acap_gid;
  bool m_syslogging;
  pid_t m_child;
  std::string m_cmd_buf;
  struct pollfd m_polling;
  volatile bool m_shutdown;
  volatile bool m_restart;
  bool m_file_logging;
  std::ofstream m_logfile;
};

int main( int argc, char * argv[] ) {
  Meta m;
  try {
    return m.run( argc, argv );
  } catch( std::exception & e ) {
    try {
      m.log( 9, e.what() );
    } catch( ... ) {
      std::cout << e.what() << std::endl;
    }
    return 51;
  } catch(...) {
    return 50;
  }
}

Meta * Meta::instance(0);

Meta::Meta() : m_listenfds(), m_ctrlfd(-1), m_remote_fd(-1), m_acap_uid( 0 ), m_acap_gid( 0 ), m_syslogging( false ), m_shutdown(false), m_restart( false ), m_file_logging( false ) {
  Meta::instance = this;
}

void Meta::open_syslog() {
  openlog( "acapd", LOG_PID|LOG_NDELAY, LOG_DAEMON );
  // Lots of people probably expect LOG_MAIL. Ultimately, the largest use probably will be mail related,
  // but that's not yet the case - currently, it's used more for Web stuff, as I write this.
  // Perhaps this'll change later, or become configurable, I dunno.
  m_syslogging = true;
}

int Meta::launch() {
  if( m_shutdown ) {
    log( 5, "Noticed requested shutdown during launch." );
    if( m_restart ) {
      std::ostringstream ss;
      for( t_listenfds::const_iterator it( m_listenfds.begin() );
	   it!=m_listenfds.end(); ++it ) {
	log( 4, "Saving " + (*it).first );
	ss << (*it).first << "\n" << (*it).second << "\n";
	long flags( fcntl( (*it).second, F_GETFL ) );
	flags |= FD_CLOEXEC;
	fcntl( (*it).second, F_SETFL, &flags );
      }
      throw std::string( ss.str() );
    } else {
      for( t_listenfds::const_iterator it( m_listenfds.begin() );
	   it!=m_listenfds.end(); ++it ) {
	log( 4, "Closing " + (*it).first );
	close( (*it).second );
      }
      std::exit( 0 );
    }
  }
  int sv[2];
  if( m_ctrlfd!=-1 ) {
    close( m_ctrlfd );
  }
  int r( socketpair( PF_UNIX, SOCK_STREAM, 0, sv ) );
  if( r!=0 ) {
    throw std::string( "Socket pair creation failed." );
  }
  m_ctrlfd = sv[0];
  m_remote_fd = sv[1];
  if( 0==( m_child = fork() ) ) {
    int topfd( m_remote_fd );
    if( m_ctrlfd > topfd ) {
      topfd = m_ctrlfd;
    }
    for( int i(0); i<=topfd; ++i ) {
      if( i!=m_remote_fd ) {
	close( i );
      }
    }
    dup2( m_remote_fd, 0 );
    close( m_remote_fd );
    
    try {
      if( getuid()==0 ) {
	if( 0!=initgroups( m_options["user"].c_str(), m_acap_gid ) ) {
#define TMP_MESSAGE "L 9 Initgroups failed.\r\n"
	    write( 0, TMP_MESSAGE, sizeof(TMP_MESSAGE) );
#undef TMP_MESSAGE
	    if( 0!=setgid( m_acap_gid ) ) throw std::runtime_error( "Couldn't switch to group." );
	}
	if( 0!=setuid( m_acap_uid ) ) throw std::runtime_error( "Couldn't switch to user." );
      }
      char ** args( new char *[ m_args.size() + 3 ] );
      unsigned int i(0);
      //args[i++] = "/usr/bin/valgrind";
      //args[i++] = "--logfile=acapd-out";
      //args[i++] = "--num-callers=16";
      //args[i++] = "./acapd-real";
      //args[i++] = 0;
      args[i++] = strdup( m_options["program"].c_str() );
      for( t_args::const_iterator foo( m_args.begin() ); foo!=m_args.end(); ++foo ) {
	args[i++] = strdup( (*foo).c_str() );
      }
      args[i++] = strdup( m_options["datadir"].c_str() );
      args[i++] = 0;
      
      execv( strdup( m_options[ "program" ].c_str() ), const_cast<char * const *>(args) );
      throw std::runtime_error( "Couldn't exec." );
    } catch( std::exception & e ) {
      std::string msg( e.what() );
      write( 0, msg.c_str(), msg.length() );
      exit( 50 );
    } catch( ... ) {
      std::string msg( "Unknown exception caught before launch." );
      write( 0, msg.c_str(), msg.length() );
      exit( 50 );
    }
  }
  std::ostringstream l;
  l << "Launched child " << m_child << " local: " << m_ctrlfd << " remote: " << m_remote_fd << std::endl << std::flush;
  log( 7, l.str() );
  close( m_remote_fd );
  m_remote_fd = -1;
  m_polling.fd = m_ctrlfd;
  m_polling.events = POLLIN|POLLHUP;
  return 0;
}
  
int Meta::run( int argc, char * argv[] ) {
  signal( SIGHUP, Meta::restart );
  signal( SIGINT, Meta::shutdown );
  signal( SIGTERM, Meta::shutdown );
  signal( SIGCHLD, SIG_IGN );
  
  // Parse arguments.
  
  m_options[ "datadir" ] = "/var/lib/acap/";
  m_options[ "user" ] = "acapd";
  m_options[ "group" ] = "";
  m_options[ "program" ] = "/usr/lib/acapd/acapd-real";
  m_options[ "fork" ] = "true";
  m_options[ "log" ] = "syslog";
  m_options[ "pidfile" ] = "/var/run/acapd.pid";
  m_options[ "once" ] = "false";
  
  int i( 1 );
  for( ; i<argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if( argv[i][1] != '-' ) {
	throw std::runtime_error( "Long options only, sorry." );
      }
      std::string option( argv[i] );
      if( option.length()==2 ) {
	++i;
	break;
      }
      option = option.substr( 2 );
      if( option == "datadir" ) {
	m_options[ "datadir" ] = argv[++i];
      } else if( option == "user" ) {
	m_options[ "user" ] = argv[++i];
      } else if( option == "group" ) {
	m_options[ "group" ] = argv[++i];
      } else if( option == "program" ) {
	m_options[ "program" ] = argv[++i];
      } else if( option == "nofork" ) {
	m_options[ "fork" ] = "false";
      } else if( option == "fork" ) {
	m_options[ "fork" ] = "true";
      } else if( option == "log" ) {
	m_options[ "log" ] = argv[++i];
      } else if( option == "pidfile" ) {
	m_options[ "pidfile" ] = argv[++i];
      } else if( option == "once" ) {
	m_options[ "once" ] = "true";
      } else if( option == "saved-fds" ) {
	std::istringstream ss( argv[++i] );
	while( ss ) {
	  std::string k;
	  int fd;
	  getline( ss, k );
	  if( ss ) {
	    ss >> fd;
	  }
	  if( ss ) {
	    m_listenfds[ k ] = fd;
	  }
	}
      } else {
	std::cout << "Ooops: Dunno what " << option << " means.\n";
	throw std::runtime_error( "For usage suggestions, try the documentation." );
      }
    }
  }
  for( ; i<argc; ++i ) {
    m_args.push_back( argv[i] );
  }

  {
    struct passwd * p( getpwnam( m_options["user"].c_str() ) );
    if( !p ) {
      const char * c( m_options["user"].c_str() );
      char * ce( 0 );
      m_acap_uid = strtoul( c, &ce, 10 );
      if( ce != ( c + m_options["user"].length() ) ) {
	throw std::runtime_error( "Cannot convert username to uid." );
      }
    } else {
      m_acap_uid = p->pw_uid;
      m_acap_gid = p->pw_gid;
    }
    if( m_options["group"] != "" ) {
      struct group * g( getgrnam( m_options["group"].c_str() ) );
      if( !g ) {
	const char * c( m_options["group"].c_str() );
	char * ce( 0 );
	m_acap_gid = strtoul( c, &ce, 10 );
	if( ce != ( c + m_options["user"].length() ) ) {
	  throw std::runtime_error( "Cannot convert username to uid." );
	}
      }
    }
  }
  
  if( m_acap_uid == 0 || m_acap_gid == 0 ) {
    if( getuid() == 0 ) {
      throw std::runtime_error( "New! Improved! Won't run as root!" );
    }
  }
  
  {
    struct stat s;
    if( 0!=stat( m_options["datadir"].c_str(), &s ) ) {
      throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not available: " + strerror( errno ) );
    }
    if( !S_ISDIR( s.st_mode ) ) {
      throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not a directory." );
    }
    if( getuid()==0 ) {
      if( s.st_uid!=m_acap_uid ) {
	throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not owned by ACAPd user." );
      }
      if( s.st_gid!=m_acap_gid ) {
	throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not owned by ACAPd group." );
      }
    } else {
      if( s.st_uid!=getuid() ) {
	throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not owned by ACAPd user." );
      }
      if( s.st_gid!=getgid() ) {
	throw std::runtime_error( std::string( "Data directory '" ) + m_options["datadir"] + "' not owned by ACAPd group." );
      }
    }
  }

  
  if( m_options["fork"] == "true" ) {
    if( m_options[ "log" ] == "-" ) {
      std::cout << "You realize I'm going to be logging all over stdout?\nTry --log /file/name or --log syslog.\nFor now, I'll run in Ugly Mode." << std::flush;
    }
    pid_t self;
    if( 0 == ( self = fork() ) ) {
      setsid();
      chdir( "/" );
      if( m_options[ "log" ] != "-" ) {
	close( 0 );
	close( 1 );
	close( 2 );
      }
    } else {
      std::ofstream fs( m_options["pidfile"].c_str() );
      fs << self << std::endl << std::flush;
      fs.close();
      exit( 0 );
    }
  }
  
  if( m_options[ "log" ] == "syslog" ) {
    open_syslog();
  } else if( m_options[ "log" ] != "-" ) {
    m_logfile.open( m_options["log"].c_str(), std::ios_base::app );
    m_file_logging = true;
  }
  
  try{
    if( !m_listenfds.empty() ) {
      log( 5, "Have saved fds:" );
      for( t_listenfds::const_iterator i( m_listenfds.begin() ); i!=m_listenfds.end(); ++i ) {
	std::ostringstream ss;
	ss << "Have saved " << (*i).first << " as " << (*i).second;
	log( 5, ss.str() );
      }
    }
    launch();
    
    if( m_options[ "once" ] == "true" ) {
      m_shutdown = true;
    }
    
    for(;;) {
      int r( poll( &m_polling, 1, 5000 ) );
      if( r>0 ) {
	if( m_polling.revents&POLLIN ) {
	  command();
	}
	if( m_polling.revents&POLLHUP ) {
	  log( 8, "Control socket closed." );
	  launch();
	}
      } else if( r<0 ) {
	if( errno!=EINTR ) {
	  log( 8, std::string( "Poll died, probably nasty: " ) + strerror( errno ) );
	  throw std::string( "Whoops." );
	}
      } else {
	// Nothing happened. Check the child exists, and continue.
	if( 0!=::kill( m_child, 0 ) ) {
	  log( 8, "Hey! No child!" );
	  launch();
	}
      }
    }
  } catch( std::string & x ) {
    int nargc( argc+3 );
    char const * * nargv( new char const *[nargc] );
    std::string savedfds( x );
    log( 9, "Reexecing server." );
    char const ** targv( nargv );
    while( ( *targv++ = *argv++ ) ); // Copy across old argv.
    nargv[argc] = strdup( "--saved-fds" );
    nargv[argc+1] = savedfds.c_str();
    nargv[argc+2] = 0;
    char progname[1024];
    int namelen( readlink( "/proc/self/exe", progname, 1023 ) );
    if( namelen > 0  && namelen <= 1023 ) {
      progname[namelen] = 0;
      std::string deleted( " (deleted)" );
      if( deleted == progname+namelen-deleted.length() ) {
	progname[namelen-deleted.length()] = 0;
      }
      nargv[0] = progname;
    } else {
      if( argv[0][0]!='/' ) {
	execvp( argv[0], const_cast<char *const *>( nargv ) ); // Try the path.
	log( 9, "Whoops, cannot find progname in path" );
	exit( 0 );
      }
    }
    execv( nargv[0], const_cast<char *const *>( nargv ) );
    log( 9, "Whoops, reexec failed!" );
    log( 9, std::string( "Prog: " ) + nargv[0] + " Errno: " + strerror( errno ) );
    exit( 0 );
  }
}

void Meta::kill( bool shutdown ) {
  m_shutdown = true;
  m_restart = !shutdown;
  ::kill( m_child, SIGTERM );
  log( 8, "Okay, that's a dead un." );
}

void Meta::shutdown( int s ) {
  signal( s, Meta::shutdown );
  Meta::instance->kill( true );
}

void Meta::restart( int s ) {
  signal( s, Meta::restart );
  Meta::instance->kill( false );
}

void Meta::command() {
  char buf[1024];
  
  int i = read( m_ctrlfd, buf, sizeof(buf) );
  
  buf[i]=0;
  
  m_cmd_buf += buf;
  
  for(;;) {
    std::string::size_type s( m_cmd_buf.find( '\n' ) );
    if( std::string::npos==s ) {
      break;
    }
    std::string cmd( m_cmd_buf.substr( 0, s ) );
    m_cmd_buf = m_cmd_buf.substr( s+1 );
    char c( cmd[0] );
    cmd = cmd.substr( 2 );
    handle_command( c, cmd );
  }
}

void Meta::handle_command( char c, std::string const & what ) {
  switch( c ) {
  case 'L':
    log( what[0]-'0', what.substr(2) );
    break;
    
  case 'E':
    log( 9, "FATAL: " + what );
    m_shutdown = true;
    break;
    
  case 'S':
    log( 2, "Requested listen socket '"+what+"'" );
    do_listen( what );
    break;
    
  case 'C':
    log( 2, "Close for '" + what + "'" );
    do_close( what );
    break;
    
  default:
    log( 9, std::string("Unknown command: ") + c + " " + what );
  }
}

void Meta::do_listen( std::string const & x ) {
  std::string family;
  std::string address;
  std::string port;
  
  family = x.substr( 0, 4 );
  address = x.substr( 5, x.find_last_of( " " )-5 );
  port = x.substr( x.find_last_of( " " )+1, x.find_last_of( "/" )-x.find_last_of( " " )-1 );
  
  log( 2, "Family: '" + family + "' address: '" + address + "' port: '" + port + "'" );
  
  try {
    int s( -1 );
    try {
      t_listenfds::const_iterator it( m_listenfds.find( x ) );
      if( it==m_listenfds.end() ) {
	struct sockaddr_storage sa;
	unsigned long sa_len;
	if( family=="ipv4" ) {
	  s = socket( PF_INET, SOCK_STREAM, 0 );
	  if( s<0 ) {
	    throw std::runtime_error( std::string( "Socket failed: " )+strerror( errno ) );
	  }
	  struct sockaddr_in * sin( reinterpret_cast<struct sockaddr_in *>( &sa ) );
	  sin->sin_family = AF_INET;
	  int r( inet_pton( AF_INET, address.c_str(), &sin->sin_addr ) );
	  if( r<=0 ) {
	    throw std::runtime_error( "Bad address" );
	  }
	  std::istringstream ss( port );
	  short unsigned p;
	  ss >> p;
	  sin->sin_port = htons( p );
	  if( ss.bad() ) {
	    throw std::runtime_error( "Bad port" );
	  }
	  sa_len = sizeof( struct sockaddr_in );
	  {
	    int optval( 1 );
	    setsockopt( s, 6, SO_REUSEADDR, &optval, sizeof(optval) );
	  }
	} else if( family=="ipv6" ) {
	  s = socket( PF_INET6, SOCK_STREAM, 0 );
	  if( s<0 ) {
	    throw std::runtime_error( std::string( "Socket failed: " ) + strerror( errno ) );
	  }
	  struct sockaddr_in6 * sin6( reinterpret_cast<struct sockaddr_in6*>( &sa ) );
	  sin6->sin6_family = AF_INET6;
	  int r( inet_pton( AF_INET6, address.c_str(), &sin6->sin6_addr ) );
	  if( r<=0 ) {
	    throw std::runtime_error( "Bad address" );
	  }
	  std::istringstream ss( port );
	  short unsigned p;
	  ss >> p;
	  sin6->sin6_port = htons( p );
	  sin6->sin6_flowinfo = 0;
	  if( ss.bad() ) {
	    throw std::runtime_error( "Bad port" );
	  }
	  sa_len = sizeof(struct sockaddr_in6);
	  {
	    int optval( 1 );
	    setsockopt( s, 6, SO_REUSEADDR, &optval, sizeof(optval) );
	  }
	} else if( family=="unix" ) {
	  s = socket( PF_UNIX, SOCK_STREAM, 0 );
	  if( s<0 ) {
	    throw std::runtime_error( std::string( "Socket failed: " ) + strerror( errno ) );
	  }
	  struct sockaddr_un * sun( reinterpret_cast<struct sockaddr_un*>( &sa ) );
	  sun->sun_family = AF_UNIX;
	  strncpy( sun->sun_path, address.c_str(), UNIX_PATH_MAX-1 );
	  sa_len = sizeof( struct sockaddr_un );
	} else {
	  throw std::runtime_error( "Bad family" );
	}
	if( 0!=bind( s, reinterpret_cast<struct sockaddr *>(&sa), sa_len ) ) {
	  throw std::runtime_error( std::string( "Bind failed: " )+strerror( errno ) );
	}
	if( 0!=listen( s, 25 ) ) {
	  throw std::runtime_error( std::string( "Listen failed: " )+strerror( errno ) );
	}
      } else {
	s = (*it).second;
      }
      struct msghdr msg = {0};
      char buf[CMSG_SPACE(sizeof(s))];
      msg.msg_control = buf;
      msg.msg_controllen = sizeof(buf);
      struct cmsghdr * cmsg( CMSG_FIRSTHDR(&msg) );
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(int));
      *reinterpret_cast<int *>( CMSG_DATA(cmsg) ) = s;
      msg.msg_controllen = cmsg->cmsg_len;
      char c( '+' );
      struct iovec iov;
      iov.iov_base = &c;
      iov.iov_len = 1;
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      if( 0>sendmsg( m_ctrlfd, &msg, 0 ) ) {
	throw std::runtime_error( std::string( "Sendmsg failed:" )+strerror( errno ) );
      }
      m_listenfds[x] = s;
      return;
    } catch(...) {
      if( s>=0 ) {
	close(s);
      }
      throw;
    }
  } catch( std::exception & e ) {
    log( 9, e.what() );
  }
  
  char c( '-' );
  write( m_ctrlfd, &c, 1 );
}

void Meta::do_close( std::string const & addr ) {
  t_listenfds::iterator it( m_listenfds.find( addr ) );
  if( it!=m_listenfds.end() ) {
    std::string unlink_me;
    struct sockaddr_un sun;
    socklen_t l;
    if( 0 == getsockname( (*it).second, reinterpret_cast<struct sockaddr *>( &sun ), &l ) ) {
      if( sun.sun_family==AF_UNIX ) {
	if( sun.sun_path[0] ) {
	  unlink_me = sun.sun_path;
	}
      }
    }
    close( (*it).second );
    m_listenfds.erase( it );
    if( !unlink_me.empty() ) {
      unlink( unlink_me.c_str() );
    }
  } else {
    log( 9, "Socket '" + addr + "' does not exist." );
  }
}

void Meta::log( int prio, std::string const & what ) {
  if( m_syslogging ) {
    int slp( LOG_DEBUG );
    if( prio > 3 ) {
      slp = LOG_NOTICE;
    }
    syslog( slp, "[%d] - %s", prio, what.c_str() );
  } else if( m_file_logging ) {
    m_logfile << "[" << prio << "] - " << what << std::endl << std::flush;
  } else {
    std::cout << "LOG: " << prio << " :: " << what << std::endl << std::flush;
  }
}
