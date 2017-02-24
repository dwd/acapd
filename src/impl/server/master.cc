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

#include <infotrope/server/master.hh>
#include <infotrope/server/worker.hh>
#include <infotrope/server/session.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/datastore/xml-loader.hh>
#include <infotrope/datastore/xml-dump.hh>
#include <infotrope/utils/memsuite.hh>
#include <infotrope/datastore/notify-sink.hh>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <cstring>

using namespace Infotrope::Server;
using namespace Infotrope::Threading;
using namespace Infotrope;

Master * Master::instance( 0 );
Master * Master::master() {
  return Master::instance;
}

namespace {
  volatile int sig_resend(0);
  volatile int sig_process(0);
  
  void signal_handler( int sig ) {
    switch( sig ) {
    case SIGTERM:
    case SIGHUP:
      sig_process = sig;
      break;
    case SIGINT:
      sig_process = SIGTERM;
    };
  }
  
  int sasl_getopt( void * context, const char * plugin_name, const char * option, const char **result, unsigned * len ) {
    static std::string option_ret;
    std::string p( "/vendor.infotrope.acapd/site/sasl/" );
    if( plugin_name ) {
      p += plugin_name;
      p += "/";
    }
    try {
      option_ret = Master::master()->sasl_getopt( p, option );
      //Master::master()->log( 5, "SASL: " + p + option + " = " + option_ret );
      *result = option_ret.c_str();
      if( len ) {
	*len = option_ret.length();
      }
    } catch( std::exception & e ) {
      //Master::master()->log( 1, std::string( "sasl_getopt: " ) + e.what() );
      *result = 0;
    } catch( std::string & e ) {
      //Master::master()->log( 1, std::string( "sasl_getopt [STR]: " ) + e );
      *result = 0;      
    } catch(...) {
      //Master::master()->log( 1, "Requested SASL option " + p + "/" + option + " not found." );
      *result = 0;
    }
    // We never admit to having errors, as such.
    // It would be too embarrassing. We are a configuration server, after all.
    return SASL_OK;
  }
  
  int sasl_log( void * context, int level, const char * message ) {
    //Master::master()->log( 1, "Logging data from SASL." );
    Master::master()->log( 8-level, std::string( "SASL :: " ) + message );
    return SASL_OK;
  }
  
  sasl_callback_t g_sasl_callbacks[] = {
    {
      SASL_CB_GETOPT,
      reinterpret_cast< int(*)() >( sasl_getopt ),
      NULL
    },
    {
      SASL_CB_LOG,
      reinterpret_cast< int(*)() >( sasl_log ),
      NULL
    },
    {
      SASL_CB_LIST_END,
      NULL,
      NULL
    }
  };
  
  struct AddressData {
    std::string addr; // Text address for CP.
    std::string running_addr; // Text address for CP.
    int fd; // Current FD.
    bool modified; // This has been modified since the last look.
    int pos; // Position in pollfd array.
  };
  
  struct MasterContext : public Notify::Sink {
    MasterContext() : Notify::Sink(), changed( false ) {
    }
    
    void handle_notify( Utils::magic_ptr<Notify::Source> const & s, Utils::StringRep::entry_type const & what, Data::Modtime const &, unsigned long int ) throw() {
      using namespace Infotrope::Data;
      //Master::master()->log( 1, "Tracking change to entry '" + what.get() + "'" );
      try {
	Infotrope::Threading::Lock l__inst( lock );
	if( dynamic_cast<Dataset*>(s.ptr()) ) {
	  Utils::magic_ptr<Dataset> dset( Utils::magic_cast<Dataset>( s ) );
	  bool exists( false );
	  std::string nf;
	  std::string na;
	  std::string tf;
	  std::string ta;
	  std::string addr;
	  Utils::magic_ptr<Entry> e( dset->fetch2( what ) );
	  if( e ) {
	    //Master::master()->log( 1, "Entry exists. Good start." );
	    exists = true;
	    if( !e->exists( "vendor.infotrope.acapd.network.family" ) || !e->attr( "vendor.infotrope.acapd.network.family" )->value()->isString() || e->attr( "vendor.infotrope.acapd.network.family" )->valuestr().find( " " )!=std::string::npos ) exists=false;
	    //Master::master()->log( 1, std::string("NF: ") + (exists?"Yes":"No" ) );
	    if( exists ) {
	      nf = e->attr( "vendor.infotrope.acapd.network.family" )->valuestr();
	      if( nf!="ipv4" && nf!="ipv6" && nf!="unix" ) {
		exists = false;
	      }
	      if( exists ) {
		if( !e->exists( "vendor.infotrope.acapd.network.address" ) || !e->attr( "vendor.infotrope.acapd.network.address" )->value()->isString() ) exists=false;
		//Master::master()->log( 1, std::string("NA: ") + (exists?"Yes":"No" ) );
	      }
	      if( exists ) {
		na = e->attr( "vendor.infotrope.acapd.network.address" )->valuestr();
		if( nf=="ipv4" || nf=="ipv6" ) {
		  if( na.find_first_of( " \r\n" )!=std::string::npos ) {
		    exists = false;
		  }
		} else if( na.find_first_of( "\r\n" )!=std::string::npos ) {
		  exists=false;
		}
		if( exists && nf!="unix" ) {
		  if( e->exists( "vendor.infotrope.acapd.transport.family" ) && e->attr( "vendor.infotrope.acapd.transport.family" )->value()->isString() && e->attr( "vendor.infotrope.acapd.transport.family" )->valuestr().find_first_of( " \r\n" )==std::string::npos ) {
		    tf = e->attr( "vendor.infotrope.acapd.transport.family" )->valuestr();
		  }
		  if( e->exists( "vendor.infotrope.acapd.transport.address" ) && e->attr( "vendor.infotrope.acapd.transport.address" )->value()->isString() && e->attr( "vendor.infotrope.acapd.transport.address" )->valuestr().find_first_of( " \r\n" )==std::string::npos ) {
		    ta = e->attr( "vendor.infotrope.acapd.transport.address" )->valuestr();
		  }
		}
	      }
	    }
	    //Master::master()->log( 1, std::string("TA: ") + (exists?"Yes":"No" ) );
	    if( nf=="ipv4" || nf=="ipv6" ) {
	      if( tf.empty() || ta.empty() ) {
		exists = false;
	      }
	    }
	    if( exists ) {
	      addr = nf + ' ' + na;
	      addr += ' ' + ta + '/' + tf;
	      if( fds.find( what )==fds.end() ) {
		fds[what].addr = addr;
		fds[what].modified = true;
		fds[what].fd = -1;
		fds[what].pos = -1;
		changed = true;
	      } else {
		fds[what].addr = addr;
		fds[what].modified = true;
		changed = true;
	      }
	    }
	  }
	  //Master::master()->log( 1, std::string("Post: ") + (exists?"Yes":"No" ) );		      
	  if( !exists ) {
	    if( fds.find( what )!=fds.end() ) {
	      fds[what].addr = "";
	      fds[what].modified = true;
	      changed = true;
	    }
	  }
	}
      } catch( std::exception & e ) {
	Master::master()->log( 8, std::string("Exception during address processing: ") + e.what() );
      } catch( std::string & e ) {
	Master::master()->log( 8, "STR during address processing: " + e );
      } catch( ... ) {
	Master::master()->log( 8, "Unknown exception during address processing." );
      }
    }
    bool changed;
    typedef std::map<Utils::StringRep::entry_type, AddressData, Utils::StringRep::entry_type_less> t_fds;
    t_fds fds;
    Infotrope::Threading::Mutex lock;
  };
  
  Utils::magic_ptr<MasterContext> g_context;
}

Master::Master( int fd, std::string const & datadir ) : m_ctrlfd( fd ), m_workers(), m_datadir( datadir ) {
  Master::instance = this;
  if( m_datadir[m_datadir.size()-1]!='/' ) {
    m_datadir += '/';
  }
}

void Master::dead( int fd ) {
  Lock l__inst( m_lock );
  /*
    std::ostringstream ss;
    ss << "Worker [" << fd << "] reporting dead...";
    log( 2, ss.str() );
  */
  m_dead.push_back( fd );
}

void Master::setup_datastore() {
  using namespace Infotrope::Data;
  log( 7, "Setting up datastore..." );
  m_ds = new Datastore();
  //log( 1, "Accessed datastore." );
  bool init( false );
  if( !m_ds->exists( Path( "/" ) ) ) {
    init = true;
    log( 5, "Starting first transaction." );
    Transaction trans( false );
    log( 5, "Creating basic system datasets." );
    m_ds->create( Path( "/byowner/" ) );
    log( 5, "Committing first transaction." );
    trans.commit();
    log( 5, "Done." );
  }
  if( init ) {
    log( 5, "Bootstrapping." );
    if( m_datadir.empty() ) {
      log( 8, "Serious Problem: No datadir specified, data will not persist!" );
    } else {
      DIR * d( opendir( m_datadir.c_str() ) );
      std::map<std::string,std::string> tranfiles;
      std::map<std::string,std::string> dumpfiles;
      std::map<std::string,std::string> datastorefiles;
      Modtime latest;
      Modtime now( latest );
      if( d ) {
	for( struct dirent * de( readdir( d ) );
	     de; de = readdir( d ) ) {
	  std::string file( de->d_name );
	  if( file.substr( 0, 5 ) == "dump-" ) {
	    Modtime m( file.substr( 5, latest.asString().length() ) );
	    if ( m > latest ) latest = m;
	    dumpfiles[ m.asString() ] = file;
	  } else if( file.substr( 0, 5 ) == "tran-" ) {
	    Modtime m( file.substr( 5, latest.asString().length() ) );
	    tranfiles[ m.asString() ] = file;
	  }
	}
	closedir( d );
      } else {
	throw std::runtime_error( "Problem opening datadir: " + m_datadir + ": " + strerror( errno ) );
      }
      Modtime time_to;
      Modtime time_from;
      if( dumpfiles.empty() ) {
	log( 7, "Found no files. I hope that's okay." );
	Transaction trans; // Recording transaction...
	m_ds->create( Path( "/vendor.infotrope.acapd/site/" ) );
	trans.commit();
	log( 7, "Created a base dump. Hoorah." );
      } else {
	std::map<std::string,std::string>::const_iterator i( dumpfiles.end() );
	--i;
	bool loaded_anything;
	while ( !( loaded_anything = load_dump( std::string( m_datadir.c_str() ) + "/" + ( *i ).second, true ) ) ) {
	  time_to = Modtime( (*i).first );
	  if ( i == dumpfiles.begin() ) break;
	  --i;
	}
	if ( loaded_anything ) {
	  time_from = Modtime( ( *i ).first );
	}
      }
      for ( std::map<std::string,std::string>::const_iterator i( tranfiles.lower_bound( time_from.asString() ) );
	    i != tranfiles.upper_bound( time_to.asString() ) && i!=tranfiles.end(); ++i ) {
	log( 5, "Found transaction from " + ( *i ).first );
	if( !load_ts( std::string( m_datadir.c_str() ) + "/" + ( *i ).second ) ) {
	  break;
	}
      }
    }
  }
}

void Master::setup_sasl() {
  sasl_set_alloc( Infotrope::Memsuite::malloc, Infotrope::Memsuite::calloc, Infotrope::Memsuite::realloc, Infotrope::Memsuite::free );
  sasl_set_mutex( Infotrope::Memsuite::mutex_alloc, Infotrope::Memsuite::mutex_lock, Infotrope::Memsuite::mutex_release, Infotrope::Memsuite::mutex_free );
  int sasl_result( sasl_server_init( g_sasl_callbacks, "Infotrope" ) );
  if( sasl_result != SASL_OK ) {
    throw std::runtime_error( sasl_errstring( sasl_result, NULL, NULL ) );
  }
  Master::master()->log( 5, "SASL initialized okay." );
}

void Master::setup_addresses() {
  using namespace Infotrope::Data;
  Datastore const & ds( Datastore::datastore_read() );
  g_context = new MasterContext();
  Utils::magic_ptr<Dataset> dset( ds.dataset( Path("/vendor.infotrope.acapd/site/") ) );
  for( Dataset::const_iterator i( dset->begin() ); i!=dset->end(); ++i ) {
    Utils::magic_ptr<Notify::Source> s( Utils::magic_cast<Notify::Source>( dset ) );
    g_context->handle_notify( s, (*i).second, Modtime::modtime(), 0 );
  }
  dset->register_sink( Utils::magic_cast<Notify::Sink>( g_context ) );
}

int Master::run() {
  log( 5, "ACAP Server starting." );
  setup_datastore();
  setup_sasl();
  setup_addresses();
  signal( SIGINT, signal_handler );
  signal( SIGTERM, signal_handler );
  signal( SIGHUP, signal_handler );
  struct pollfd p[25];
  unsigned int count(0);
  unsigned int fallback_attempt( 0 );
  for(;;) {
    {
      Infotrope::Threading::Lock l__inst( g_context->lock );
      if( g_context->changed ) {
	unsigned int counter(0);
	bool change( false );
	for( MasterContext::t_fds::iterator i( g_context->fds.begin() );
	     i!=g_context->fds.end(); ++i ) {
	  if( (*i).second.modified ) {
	    if( !(*i).second.running_addr.empty() ) {
	      log( 5, "Closing: " + (*i).second.running_addr );
	      close( (*i).second.running_addr );
	      ::close( (*i).second.fd );
	      (*i).second.fd = -1;
	      (*i).second.running_addr = "";
	    }
	    if( !(*i).second.addr.empty() ) {
	      log( 5, "Opening: " + (*i).second.addr );
	      (*i).second.fd = listen( (*i).second.addr );
	      if( (*i).second.fd >= 0 ) {
		(*i).second.running_addr = (*i).second.addr;
	      }
	    }
	    (*i).second.modified = false;
	    change = true;
	  }
	}
	if( change ) {
	  for( MasterContext::t_fds::iterator i( g_context->fds.begin() );
	       i!=g_context->fds.end(); ++i ) {
	    if( !(*i).second.running_addr.empty() ) {
	      std::ostringstream ss;
	      ss << (*i).first.get() << ": Listening on " << (*i).second.running_addr << " as " << (*i).second.fd << ", position " << counter;
	      log( 5, ss.str() );
	      (*i).second.pos = counter;
	      p[counter].fd = (*i).second.fd;
	      p[counter].events = POLLIN|POLLHUP;
	      ++counter;
	    }
	  }
	  count = counter;
	}
	g_context->changed = false;
      }
    }
    if( count==0 ) {
      Data::Transaction trans;
      Utils::magic_ptr<Data::Entry> e( new Data::Entry( "fallback" ) );
      if( fallback_attempt==0 ) {
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.family", "ipv6" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.address", "::0" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.family", "tcp" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.address", "674" ) );
	fallback_attempt++;
      } else if( fallback_attempt==1 ) {
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.family", "ipv4" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.address", "0.0.0.0" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.family", "tcp" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.address", "674" ) );
	fallback_attempt++;
      } else {
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.family", "ipv4" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.network.address", "0.0.0.0" ) );
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.family", "tcp" ) );
	std::ostringstream ss;
	ss << ( fallback_attempt++ - 1 ) << 674;
	e->add( new Data::Attribute( "vendor.infotrope.acapd.transport.address", ss.str() ) );
      }
      Data::Datastore::datastore().dataset( Data::Path( "/vendor.infotrope.acapd/site/" ) )->add2( e );
      log( 7, "No listening connection, attempting fallback." );
      trans.commit();
    } else {
      fallback_attempt = 0;
      int r( poll( p, count, m_workers.empty()?-1:250 ) );
      if( r<0 ) {
	if( errno==EINTR ) {
	  log( 0, "EINTR during poll. Interesting." );
	} else {
	  log( 9, "Whoops. Nasty thingy during poll." );
	}
      } else if( r>0 ) {
	for( unsigned int i(0); i!=count; ++i ) {
	  if( p[i].revents & POLLIN ) {
	    //log( 0, "Cooool. Connection." );
	    struct sockaddr sa;
	    socklen_t l( sizeof(sa) );
	    int s( accept( p[i].fd, &sa, &l ) );
	    if( s>=0 ) {
	      //log( 0, "Valid connection, good." );
	      m_workers[s] = new Worker( s );
	      m_workers[s]->start();
	    }
	  }
	}
      }
    }
    if( sig_process!=0 ) {
      for( t_workers::iterator i( m_workers.begin() );
	   i!=m_workers.end(); ++i ) {
	(*i).second->post_message( Worker::Shutdown );
      }
    }
    {
      Lock l__inst( m_lock );
      while( !m_dead.empty() ) {
	int tmp( m_dead[0] );
	std::ostringstream ss;
	ss << "Worker [" << tmp << "] :: ";
	m_dead.pop_front();
	Utils::magic_ptr<Worker> w( m_workers[tmp] );
	if( w ) {
	  //log( 2, ss.str() + "Stopping." );
	  w->stop();
	  //log( 2, ss.str() + "Deleting." );
	  w.zap();
	  log( 2, ss.str() + "Killed, erasing entry." );
	}
	m_workers.erase(tmp);
      }
      if( sig_process!=0 ) {
	if( sig_process==SIGTERM ) {
	  log_except( "Got SIGTERM, exiting." );
	}
	break;
      }
      if( m_full_dump_rq ) {
	m_full_dump_rq = false;
	Infotrope::Data::Dump d;
	log( 5, "Performing full dump" );
	d.dumper();
      }
    }
  }
  log( 2, "ACAP Server shutting down." );
  shutdown_fast();
  return 0;
}

void Master::full_dump() {
  Threading::Lock l__inst( m_lock );
  m_full_dump_rq = true;
}

void Master::send_control( char what, std::string const & msg ) const {
  std::string line;
  line += what;
  line += ' ';
  line += msg;
  line += '\n';
  if( std::string::npos==line.find( '\n' ) ) {
    throw std::runtime_error( "Invalid control protocol line." );
  }
  ::write( m_ctrlfd, line.c_str(), line.length() );
}

int Master::listen( std::string const & address ) {
  send_control( 'S', address );
  char c;
  struct iovec iov;
  iov.iov_base = &c;
  iov.iov_len = 1;
  struct msghdr msg = {0};
  char buf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = buf;
  msg.msg_controllen = sizeof(buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  struct cmsghdr * h( CMSG_FIRSTHDR( &msg ) );
  h->cmsg_len = CMSG_LEN(sizeof(int));
  h->cmsg_level = SOL_SOCKET;
  h->cmsg_type = SCM_RIGHTS;
  *reinterpret_cast<int*>( CMSG_DATA(h) ) = -1;
  
  int r( recvmsg( m_ctrlfd, &msg, 0 ) );
  if( r>=0 ) {
    for( struct cmsghdr * h( CMSG_FIRSTHDR( &msg ) );
	 h != NULL;
	 h = CMSG_NXTHDR( &msg, h ) ) {
      if( h->cmsg_level == SOL_SOCKET && h->cmsg_type == SCM_RIGHTS ) {
	return *(reinterpret_cast<int *>( CMSG_DATA( h ) ) );
      }
    }
  }
  return -1;
}

void Master::close( std::string const & addr ) {
  send_control( 'C', addr );
}

void Master::log_except( std::string const & w ) const {
  send_control( 'E', "Fatal exception follows" );
  log( 9, w );
}

void Master::log( int prio, std::string const & w ) const {
  if( prio > 9 ) prio=9;
  std::string::size_type to( w.find( '\n' ) );
  std::string::size_type from(0);
  while( to!=std::string::npos ) {
    log_real( prio, ( from==0 ? "" : " ... " ) + w.substr( from, to-from ) );
    from = to + 1;
    to = w.find( '\n', from );
  }
  log_real( prio, ( from==0 ? "" : " ... " ) + w.substr( from ) );
}

void Master::log_real( int prio, std::string const & w ) const {
  if( std::string::npos!=w.find( '\n' ) ) {
    log_real(  8, "! Invalid log line !" );
    return;
  }
  std::string t;
  t += '0'+prio;
  t += ' ';
  t += w;
  send_control( 'L', t );
}

Master::~Master() {
  shutdown_fast();
  log( 8, "Master shutdown." );
}

void Master::shutdown_fast() {
  for( t_workers::iterator i( m_workers.begin() ); i!=m_workers.end(); ++i ) {
    if( (*i).second ) {
      (*i).second->post_message( Worker::Shutdown );
    }
  }
  for( t_workers::iterator i( m_workers.begin() ); i!=m_workers.end(); ++i ) {
    if( (*i).second ) {
      (*i).second->stop();
      (*i).second.zap();
    }
  }
  m_workers.clear();
  m_dead.clear();
}

bool Master::exists( int fd ) const {
  Lock l__inst( m_lock );
  return m_workers.find( fd )!=m_workers.end();
}

Infotrope::Utils::magic_ptr<Worker> Master::get( int fd ) const {
  Lock l__inst( m_lock );
  t_workers::const_iterator i( m_workers.find( fd ) );
  if( i == m_workers.end() ) {
    return Utils::magic_ptr<Worker>();
  }
  return (*i).second;
}

bool Master::load_ts( std::string const & file ) {
  return load_dump( file, true );
}

bool Master::load_dump( std::string const & file, bool record ) {
  log( 5, "Loading dump file "+file );
  try {
    Infotrope::Data::Loader l;
    std::ifstream ifs( file.c_str() );
    while ( ifs ) {
      std::string s;
      getline( ifs, s );
      s += "\n";
      if ( ifs ) {
	l.parse( s );
      }
    }
    l.parse( "", true );
    if ( !l.sane() ) {
      log( 8, "Load of "+file+" FAILED : File incomplete?" );
    } else {
      log( 8, "Load of " + file + " SUCCEEDED" );
    }
    return l.sane();
  } catch( std::exception & e ) {
    log( 8, "Load of "+file+" FAILED : " + e.what() );
  }
  return false;
}

std::string Master::sasl_getopt( std::string const & path, std::string const & option ) {
  // Bulldoze through trying to find an option.
  Data::Datastore & ds( Data::Datastore::datastore() );
  Threading::ReadLock l__inst( ds.lock() );
  Utils::magic_ptr<Data::Dataset> dset( ds.dataset( path ) );
  Utils::magic_ptr<Data::Entry> e( dset->fetch2( option ) );
  if( e ) {
    return e->attr( "vendor.infotrope.acapd.value" )->valuestr();
  }
  throw std::runtime_error( "No SASL option." );
}

#ifdef INFOTROPE_ENABLE_XPERSIST
Infotrope::Utils::magic_ptr<Session> const & Master::session( std::string const & name ) {
  Infotrope::Threading::Lock l__inst( m_session_lock );
  if( m_sessions.empty() ) {
    m_sessions[""] = 0; // Nasty, but to hell with it.
  }
  t_sessions::const_iterator i( m_sessions.find( name ) );
  if( i == m_sessions.end() ) return m_sessions[""];
  return (*i).second;
}

void Master::session( Utils::magic_ptr<Session> const & s ) {
  Infotrope::Threading::Lock l__inst( m_session_lock );
  m_sessions[s->name()] = s;
}
#endif

int main( int argc, char ** argv ) {
  try {
    Master m( 0, argv[1] );
    try {
      return m.run();
    } catch( std::exception & e ) {
      m.log_except( e.what() );
    } catch( std::string & e ) {
      m.log_except( "String: " + e );
    } catch( ... ) {
      m.log_except( "Unknown exception" );
    }
    m.log( 5, "ACAP server shutdown complete." );
  } catch(...) {
    return 1;
  }
  return 0;
}
