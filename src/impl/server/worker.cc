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

#include <infotrope/server/worker.hh>
#include <infotrope/server/master.hh>
#include <infotrope/server/command.hh>
#include <infotrope/server/context.hh>
#ifdef INFOTROPE_ENABLE_XPERSIST
#include <infotrope/server/session.hh>
#endif
#include <infotrope/datastore/modtime.hh>
#include <string>
#include <sstream>
#include <sys/poll.h>
#include <errno.h>

// inet_ntop, getsockname, getpeername
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// close. Dunno why I missed this.
#include <unistd.h>

// UNIX SASL EXTERNAL stuff.
#include <pwd.h>
#include <sys/un.h>

// strerror
#include <cstring>

using namespace Infotrope::Server;
using namespace Infotrope::Threading;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;

namespace {
  class fatal_error : public std::runtime_error {
  public:
    fatal_error( std::string const & s ) : std::runtime_error( s ) {
    }
  };
}

Worker::Worker( int fd ) : Thread(), m_fd( fd ), m_msgs(), m_msglock(), m_tcp(), m_dead( false ) {
  m_tcp = new Infotrope::Stream::tcpstream( m_fd );
  clear_command_state();
  push_scope( INIT );
  push_scope( PREAUTH );
}

void Worker::log( int l, std::string const & s ) const {
  std::ostringstream ss;
  ss << "Worker [" << fd() << "]: " << s;
  Master::master()->log( l, ss.str() );
}

Worker::t_msg Worker::get_message( bool wait ) {
  Lock l__inst( m_msglock );
  if( 0==m_msgs.size() ) {
    if( !wait ) {
      return NoMessage;
    }
    m_msglock.wait( l__inst );
  }
  t_msg tmp( m_msgs[0] );
  m_msgs.pop_front();
  return tmp;
}

void Worker::post_message( Worker::t_msg m ) {
  Lock l__inst( m_msglock );
  m_msgs.push_back( m );
  m_msglock.signal( l__inst );
}

void Worker::banner() {
  Infotrope::Utils::magic_ptr<Infotrope::Token::List> b( new Infotrope::Token::List() );
  b->add( "*" );
  b->add( "ACAP" );
  Infotrope::Utils::magic_ptr<Infotrope::Token::List> impl( new Infotrope::Token::PList() );
  impl->add( "IMPLEMENTATION" );
  impl->add( new Infotrope::Token::QuotedString( std::string( "Infotrope ACAP Server, version " ) + INFOTROPE_PACKAGE_VERSION + ", Copyright 2002-2004 Dave Cridland <dave@cridland.net>" ) );
  b->add( magic_cast<Infotrope::Token::Token>(impl) );
  const char * mechlist(0);
  int count;
  if( SASL_OK==sasl_listmech( m_sasl_conn, 0, "(SASL \"", "\" \"", "\")", &mechlist, 0, &count ) ) {
    if( mechlist ) {
      b->add( new Token::Atom( mechlist ) );
    }
  }
#ifdef INFOTROPE_ENABLE_XUNIQUE
  {
    magic_ptr<Infotrope::Token::Token> l( new Infotrope::Token::PList() );
    l->toList().add( "XUNIQUE" );
    b->add( l );
  }
#endif
#ifdef INFOTROPE_ENABLE_XPERSIST
  {
    magic_ptr<Infotrope::Token::Token> l( new Infotrope::Token::PList() );
    l->toList().add( "XPERSIST" );
    b->add( l );
  }
#endif
#ifdef INFOTROPE_ENABLE_STARTTLS
  if( m_tcp->bits()==0 ) {
    magic_ptr<Infotrope::Token::Token> l( new Infotrope::Token::PList() );
    l->toList().add( "STARTTLS" );
    b->add( l );
  }
#endif
#ifdef INFOTROPE_HAVE_ZLIB_H
  {
    magic_ptr<Infotrope::Token::Token> l( new Infotrope::Token::PList() );
    l->toList().add( "COMPRESS" );
    l->toList().add( "DEFLATE" );
    b->add( l );
  }
#endif
  send( b, true );
}

void Worker::send( Infotrope::Utils::magic_ptr<Infotrope::Token::List> const & t, bool f ) {
  Lock l__inst( m_sendlock );
  if( !m_tcp ) {
    return;
  }
  if( !(*m_tcp) ) {
    return;
  }
  try {
    if( t ) {
      (*m_tcp) << (*t) << "\r\n";
    }
    if( f ) {
      (*m_tcp) << std::flush;
    }
  } catch(...) {
    shutdown();
  }
  if( !(*m_tcp) ) {
    shutdown();
  }
}

namespace {
  std::string sockaddr_to_sasl( struct sockaddr * s ) {
    std::string iplocalport;
    char scratch[1024];
    if( s->sa_family==AF_INET ) {
      struct sockaddr_in * sin( reinterpret_cast<struct sockaddr_in *>( s ) );
      if( 0!=inet_ntop( AF_INET, reinterpret_cast<void*>(&(sin->sin_addr)), scratch, 1024 ) ) {
	std::ostringstream iplocalports;
	iplocalports << scratch << ';' << ntohs( sin->sin_port );
	iplocalport = iplocalports.str();
      }
    } else if( s->sa_family==AF_INET6 ) {
      struct sockaddr_in6 * sin6( reinterpret_cast<struct sockaddr_in6 *>( s ) );
      if( 0!=inet_ntop( AF_INET6, reinterpret_cast<void*>(&(sin6->sin6_addr)), scratch, 1024 ) ) {
	std::ostringstream iplocalports;
	iplocalports << scratch << ';' << ntohs( sin6->sin6_port );
	iplocalport = iplocalports.str();
      }
    }
    return iplocalport;
  }
}

void Worker::sasl_setup() {
  struct sockaddr s;
  socklen_t sl( sizeof( s ) );
  std::string iplocalport;
  if( 0==getsockname( m_fd, &s, &sl ) ) {
    iplocalport = sockaddr_to_sasl( &s );
    if( s.sa_family==AF_UNIX ) {
      log( 5, std::string("Connection to ") + reinterpret_cast<struct sockaddr_un *>( &s )->sun_path );
    } else {
      log( 5, "Connection to " + iplocalport );
    }
  }
  sl = sizeof( s );
  std::string ipremoteport;
  struct passwd * p( 0 );
  if( 0==getpeername( m_fd, &s, &sl ) ) {
    ipremoteport = sockaddr_to_sasl( &s );
    if( s.sa_family==AF_UNIX ) {
      struct ucred u;
      socklen_t l( sizeof( u ) );
      int r( getsockopt( m_fd, SOL_SOCKET, SO_PEERCRED, &u, &l ) );
      if( r==0 ) {
	p = getpwuid( u.uid );
	std::ostringstream ss;
	ss << "Got UNIX EXTERNAL from pid " << u.pid << ", uid " << u.uid << " authid " << ( p ? p->pw_name : "<none>" );
	log( 5, ss.str() );
      } else {
	log( 5, std::string( "SCM_CREDENTIALS failed: " ) + strerror( errno ) );
      }
    } else {
      log( 5, "Connection from " + ipremoteport );
    }
  }
  m_sasl_conn = 0;
  int sasl_return( sasl_server_new(
				   "acap", 
				   // Service name.
				    NULL, 
				   // serverFQDN (Use gethostbyname())
				    NULL,
				   // Default user realm. (None)
				    ( iplocalport.empty() ? NULL : iplocalport.c_str() ),
				   // iplocalport. (As figured out above.)
				    ( ipremoteport.empty() ? NULL : ipremoteport.c_str() ),
				   // ipremoteport. (Likewise.)
				   NULL,
				   // Callbacks. None yet.
				    SASL_SUCCESS_DATA,
				   // Flags, we support a final SASL splurge.
				    &m_sasl_conn ) );
  /*
    TODO:
    Really, there are several values here that we ought to be changing on a per-listen-port basis.
    For instance, the server FQDN, user realm, and some of the callbacks.
    This could mean we could do virtual serving, in effect.
    I'm quite keen on the idea, but sadly I lack the time. It's also, I suspect, heavily worthless without
    the additional facility of groups - which I really want to implement using Alexey Melnikov/Steve Hole's
    authorization datasets - a pretty powerful, and, dare I say it, cool method of storing a full authorization
    database in ACAP itself.
    This in turn can't be implemented until I've fathomed out a sane way of handling 'active clients'.
  */
  if( sasl_return!=SASL_OK ) {
    log( 5, "SASL failed for this connection." );
    throw fatal_error( std::string( "SASL :: " ) + sasl_errstring( sasl_return, 0, 0 ) );
  }
  sasl_security_properties_t secprops;
  secprops.min_ssf = 0;
  secprops.max_ssf = 0x7FFF;
  secprops.maxbufsize = 2048;
  secprops.property_names = NULL;
  secprops.property_values = NULL;
  secprops.security_flags = 0;
  sasl_setprop( m_sasl_conn, SASL_SEC_PROPS, &secprops );
  if( p ) {
    std::string name( p->pw_name );
    if( "root" == name ) {
      // Hackish. The idea is that if you connect via AF_UNIX sockets as root, you can authenticate
      // instantly as 'admin', the ACAP superuser. It's really bootstrap support.
      // Essentially, when you spin up the server for the first time, you need some way of configuring
      // it, and since all the configuration data is in the server itself, there needs to be a way to bypass
      // authentication securely, in order to configure it. This is it.
      name = "admin";
    }
    sasl_setprop( m_sasl_conn, SASL_AUTH_EXTERNAL, name.c_str() );
  }
}

void * Worker::runtime() {
  try {
    try {
      sasl_setup();
      banner();
      struct pollfd p;
      p.fd = m_fd;
      p.events = POLLIN|POLLHUP|POLLERR;
      for(;;) {
	switch( get_message( false ) ) {
	case Shutdown:
	  shutdown();
	  log( 2, "Worker quitting, processed shutdown message." );
	  return 0;
	  
	case Terminated:
	  {
	    std::stack<t_commands::iterator> s;
	    for( t_commands::iterator i( m_commands.begin() ); i!=m_commands.end(); ++i ) {
	      if( (*i).second->dead() ) {
		(*i).second->stop();
		(*i).second.zap();
		s.push( i );
	      }
	    }
	    while( !s.empty() ) {
	      t_commands::iterator i( s.top() );
	      s.pop();
	      m_commands.erase( i );
	    }
	  }
	default:
	  break;
	}
	if( m_dead ) {
	  log( 2, "Worker quitting, dead already." );
	  return 0;
	}
	if( !*m_tcp ) {
	  shutdown();
	  log( 2, "Worker quitting, stream dead." );
	  return 0;
	}
	
	int r( poll( &p, 1, 100 ) );
	
	if( r<0 ) {
	  if( errno!=EINTR ) {
	    log( 3, std::string("Poll screwed up: ") + std::strerror( errno ) );
	    Master::master()->dead( m_fd );
	    return 0;
	  }
	} else if( r>0 ) {
	  if( p.revents&(POLLHUP|POLLERR) ) {
	    log( 3, "Connection dead." );
	    Master::master()->dead( m_fd );
	    return 0;
	  } else if( p.revents&POLLIN ) {
	    waiting();
	  }
	}
      }
    } catch( std::exception & e ) {
      magic_ptr<Token::List> l( new Token::List );
      l->add( "*" );
      l->add( "BYE" );
      l->add( std::string( "Fatal error on this connection :: " ) + e.what() );
      send( l, true );
    } catch( std::string & e ) {
      magic_ptr<Token::List> l( new Token::List );
      l->add( "*" );
      l->add( "BYE" );
      l->add( std::string( "Fatal error on this connection :: " ) + e );
      send( l, true );
    } catch( ... ) {
      magic_ptr<Token::List> l( new Token::List );
      l->add( "*" );
      l->add( "BYE" );
      l->add( std::string( "Fatal error on this connection :: <Unknown>" ) );
      send( l, true );
    }
  } catch( ... ) {
  }
  log( 9, "Connection died with exception." );
  Master::master()->dead( m_fd );
  return 0;
}

void Worker::feed_me( Command * c ) {
  if( c!=m_command.ptr() ) {
    throw std::runtime_error( "Request to feed non-current command." );
  }
  //Master::master()->log( 1, "Feed requested." );
  m_feeding = true;
}

void Worker::waiting() {
  //Master::master()->log( 2, "Wait loop started." );
  do {
    //Master::master()->log( 2, "Wait loop iteration entered." );
    try {
      if( m_command_dead ) {
	if( !m_eol_seen || ( m_marker && m_marker->nonSync() ) ) {
	  //Master::master()->log( 1, "Spoool." );
	  parse_spool();
	  command_failure();
	  //Master::master()->log( 1, "Okay, continuing." );
	} else {
	  clear_command_state();
	}
      }
      if( !m_command_dead ) {
	parse_line();
	if( !m_command ) {
	  if( m_eol_seen ) {
	    //Master::master()->log( 1, "Spawning new command." );
	    new_command();
	  }
	}
	if( m_command ) {
	  //Master::master()->log( 1, "Have command." );
	  if( m_feeding ) {
	    //Master::master()->log( 1, "Am feeding." );
	    if( m_marker ) {
	      if( !m_marker->nonSync() ) {
		magic_ptr<List> l( new List );
		l->add( "+" );
		l->add( new QuotedString( "You should consider using non-synchronising literals with AUTHENTICATE, it's much better." ) );
		send( l, true );
	      }
	    } else {
	      //Master::master()->log( 1, "Feeding the beast." );
	      m_feeding = m_command->feed();
	      if( m_feeding ) {
		reset_parser();
	      } else {
		clear_command_state();
	      }
	    }
	  } else {
	    //Master::master()->log( 1, "Am Parsing." );
	    if( m_command->parse( !m_marker ) ) {
	      if( !m_marker ) {
		t_commands::iterator i( m_commands.find( m_toks->get(0).asString() ) );
		if( i!=m_commands.end() ) {
		  if( (*i).second ) {
		    (*i).second->stop();
		    (*i).second.zap();
		  }
		  m_commands.erase( i );
		}
		m_command->execute();
		if( m_feeding ) {
		  reset_parser();
		} else {
		  if( !m_command->dead() ) {
		    m_commands[m_toks->get(0).asString()] = m_command;
		  }
		  clear_command_state();
		}
	      } else {
		if( !m_marker->nonSync() ) {
		  magic_ptr<List> l( new List );
		  l->add( "+" );
		  l->add( new QuotedString( "Go Ahead" ) );
		  send( l, true );
		}
	      }
	    } else {
	      command_failure();
	    }
	  }
	}
      }
    } catch( fatal_error & e ) {
      throw;
    } catch( std::exception & e ) {
      //Master::master()->log( 1, "Foo, exception caught." );
      magic_ptr<List> l( new List );
      if( m_toks && m_toks->length() ) {
	if( m_toks->get(0).isAtom() && m_toks->get(0).toAtom().value()!="+" ) {
	  l->add( m_toks->ptr( 0 ) );
	} else {
	  l->add( "*" );
	}
      } else {
	l->add( "*" );
      }
      l->add( "BAD" );
      if( m_toks ) {
	magic_ptr<List> pl( new PList );
	pl->add( "XORIGTOKEN" );
	pl->add( magic_cast<Token::Token>( m_toks ) );
	l->add( magic_cast<Token::Token>( pl ) );
      }
      l->add( new QuotedString( std::string("(caught by Worker) ") + e.what() ) );
      send( l, true );
      command_failure();
    }
  } while( m_tcp && m_tcp->rdbuf()->in_avail()!=0 );
  //Master::master()->log( 2, "Wait loop ended." );
}

void Worker::parse_spool() {
  parse_line( true );
}

void Worker::clear_command_state() {
  m_command.zap();
  m_feeding = false;
  reset_parser();
}

void Worker::reset_parser() {
  m_marker.zap();
  m_toks.zap();
  while( !m_active.empty() ) {
    m_active.pop();
  }
  m_quoted = false;
  m_escaped = false;
  m_skip_space = false;
  m_eol_seen = false;
  m_command_dead = false;
  m_spool_len = 0;
  m_buf.clear();
}

void Worker::command_failure() {
  m_command.zap();
  //Master::master()->log( 1, " *** COMMAND FAILURE *** " );
  m_command_dead = true;
  if( m_eol_seen && ( !m_marker || !m_marker->nonSync() ) ) {
    //Master::master()->log( 1, "I do NOT need to spool." );
    clear_command_state();
  }
}

void Worker::stuff_token( magic_ptr<Token::Token> const &p ) {
  bool init( false );
  if( !m_toks ) {
    m_toks = new List();
    m_active.push( m_toks );
    init = true;
  }
  m_active.top()->add( p );
  if( p->isList() ) {
    m_active.push( magic_cast<List>( p ) );
  }
  if( init ) {
    if( m_feeding ) {
      return;
    }
    if( p->isInteger() ) {
      // QUietly ignore this.
      return;
    }
    if( !p->isAtom() ) {
      throw std::runtime_error( "Tag must be an atom." );
    } else {
      if( p->toAtom().value()=="*" || p->toAtom().value()=="+" ) {
	throw std::runtime_error( "Tag must not be * or +, or you'll get blindingly confused." );
      }
    }
  }
}

void Worker::pop_list() {
  if( m_active.size()==1 ) {
    throw std::runtime_error( "Attempted to pop last list off stack." );
  }
  m_active.pop();
}

void Worker::stuff_list() {
  stuff_token( magic_ptr<Token::Token>( new PList() ) );
}

void Worker::stuff_atom() {
  std::string buf( m_buf );
  m_buf.clear();
  if( m_marker ) {
    // No, not an Atom at all, it's a literal string.
    m_marker.zap();
    stuff_token( magic_ptr<Token::Token>( new LiteralString( buf ) ) );
    return;
  }
  bool digits( true );
  long int foo(0);
  if( m_toks ) {
    for( std::string::size_type i(0); i<buf.length(); ++i ) {
      if( !std::isdigit( buf[i] ) ) {
	digits = false;
	break;
      } else {
	foo *= 10;
	foo += buf[i]-'0';
      }
    }
  } else {
    // Catch all-digit tag.
    digits = false;
  }
  
  if( digits ) {
    stuff_token( magic_ptr<Token::Token>( new Integer( foo ) ) );
  } else {
    magic_ptr<Token::Token> a( new Atom( buf ) );
    if( a->toAtom().value()=="NIL" ) {
      stuff_token( magic_ptr<Token::Token>() );
    } else {
      stuff_token( a );
    }
  }
}

void Worker::stuff_qs() {
  stuff_token( magic_ptr<Token::Token>( new QuotedString( m_buf ) ) );
  m_buf = "";
}

void Worker::stuff_literal() {
  stuff_token( magic_ptr<Token::Token>( new LiteralString( m_buf ) ) );
  m_buf = "";
}

void Worker::stuff_literalmarker() {
  std::istringstream ss( m_buf );
  char junk;
  ss.get( junk );
  unsigned long l;
  ss >> l;
  ss.get( junk );
  bool ns( junk=='+' );
  m_marker = new LiteralMarker( l, ns );
  stuff_token( magic_cast<Token::Token>( m_marker ) );
  m_buf = "";
  m_spool_len = l;
}

void Worker::parse_line( bool spooling ) {
  bool first(true);
  
  if( m_spool_len>0 ) {
    // Read in the data.
    if( spooling ) {
      m_tcp->ignore( m_spool_len );
      m_spool_len = 0;
      m_skip_space = true;
      m_marker.zap();
      if( !*m_tcp ) {
	return;
      }
    } else {
      for(bool breakout(false); !breakout; ) {
	char b[4096];
	unsigned long int tl( m_spool_len );
	if( tl>4096 ) {
	  tl = 4096;
	}
	unsigned long int got( m_tcp->readsome( b, tl ) );
	if( got>0 ) {
	  m_spool_len -= got;
	  m_buf += std::string( b, b+got );
	}
	if( m_spool_len==0 ) {
	  breakout = true;
	} else if( got==0 ) {
	  char c;
	  m_tcp->get( c );
	  if( !*m_tcp ) {
	    return;
	  }
	  m_buf += c;
	  --m_spool_len;
	}
      }
      m_skip_space = true;
    }
  }
  
  for(bool breakout(false); !breakout; ) {
    if( first ) {
      first = false;
    } else {
      if( m_tcp->rdbuf()->in_avail()==0 ) {
	return;
      }
    }
    
    char c;
    m_tcp->get(c);
    
    if(!*m_tcp) {
      post_message( Shutdown );
      return;
    }
    
    if( m_quoted ) {
      if( m_escaped ) {
	m_escaped = false;
	m_buf += c;
      } else {
	switch( c ) {
	case '\\':
	  m_escaped = true;
	  continue;
	case '"':
	  m_quoted = false;
	  m_skip_space = true;
	  if( !spooling) stuff_qs();
	  continue;
	case '\n':
	  {
	    throw fatal_error( "EOL found in quoted string." );
	  }
	default:
	  m_buf += c;
	}
      }
    } else {
      switch( c ) {
      case '"':
	m_quoted = true;
	continue;
      case '(':
      case ')':
	m_skip_space = true;
      case ' ':
      case '\n':
	if( c=='\n' ) {
	  m_eol_seen = true;
	}
	//Master::master()->log( 1, "Space, list, or LF." );
	if( m_buf.length() ) {
	  if( m_buf[0]=='{' ) {
	    stuff_literalmarker();
	  } else {
	    if( !spooling ) stuff_atom();
	  }
	} else if( !m_skip_space ) {
	  //Master::master()->log( 1, "Skip space." );
	  if( m_marker ) {
	    //Master::master()->log( 1, "Stuffing Atom" );
	    if( !spooling ) stuff_atom();
	  } else {
	    //Master::master()->log( 1, "Empty token, error." );
	    throw std::runtime_error( "Empty token - probably too many spaces." );
	  }
	}
	//Master::master()->log( 1, "Here." );
	if( c=='\n' ) {
	  if( spooling ) return;
	  if( m_active.size()!=1 && !m_marker ) {
	    throw fatal_error( "More '(' than ')' in line?" );
	  }
	  //Master::master()->log( 1, "Registered EOL, returning." );
	  return;
	} else if( c=='(' ) {
	  if( !spooling ) stuff_list();
	} else if( c==')' ) {
	  if( !spooling ) pop_list();
	}
	//Master::master()->log( 1, "End." );
	continue;
	
      case '\r':
	continue;
	
      default:
	m_buf += c;
	m_skip_space = false;
      }
    }
  }
}

void Worker::new_command() {
  try {
    //Master::master()->log( 3, "Spawning new command." );
    if( m_toks->length() < 1 ) {
      Infotrope::Utils::magic_ptr<Infotrope::Token::List> l( new Infotrope::Token::List );
      l->add( "*" );
      l->add( "BAD" );
      l->add( new Infotrope::Token::QuotedString( "Empty line?" ) );
      send( l, true );
      command_failure();
    } else if( m_toks->length() < 2 ) {
      Infotrope::Utils::magic_ptr<Infotrope::Token::List> l( new Infotrope::Token::List );
      l->add( m_toks->ptr(0) );
      l->add( "BAD" );
      l->add( new Infotrope::Token::QuotedString( "Missing command?" ) );
      send( l, true );
      command_failure();
    } else if( !m_toks->get(1).isAtom() ) {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "BAD" );
      l->add( new Infotrope::Token::QuotedString( m_toks->get( 1 ).asString() + " is not an Atom." ) );
      send( l, true );
      command_failure();
    } else {
      log( 3, "Trying to spawn command " + istring_convert( m_toks->get( 1 ).toAtom().value() ) );
      Command * c( 0 );
      for( t_scopes::const_iterator i( m_scopes.begin() ); i!=m_scopes.end(); ++i ) {
	c = Command::create( m_toks->get( 1 ).toAtom().value(), (*i), *this );
	if( c ) {
	  // m_command_dead = false;
	  m_command = c;
	  return;
	}
      }
      
      log( 3, "Bad command " + istring_convert( m_toks->get( 1 ).toAtom().value() ) );
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "BAD" );
      l->add( Utils::istring_convert( "Unknown command " + m_toks->get( 1 ).toAtom().value() ) );
      send( l, true );
      command_failure();
    }
  } catch( std::exception & e ) {
    log( 8, std::string("Exception during Worker::new_command(): ") + e.what() );
    command_failure();
    throw;
  }
}

void Worker::shutdown() {
  if( !m_dead ) {
    for( t_commands::iterator i( m_commands.begin() ); i!=m_commands.end(); ++i ) {
      (*i).second->stop();
      (*i).second.zap();
    }
    m_commands.clear();
    for( t_contexts::iterator i( m_contexts.begin() ); i!=m_contexts.end(); ++i ) {
      (*i).second->shutdown();
      (*i).second.zap();
    }
    m_contexts.clear();
    if( m_tcp && *m_tcp ) {
      try {
	magic_ptr<List> l( new List );
	l->add( "*" );
	l->add( "BYE" );
	l->add( new QuotedString( "Connection closing" ) );
	send( l, true );
      } catch( ... ) {
	// Ignore any exception, it's not very interesting.
      }
    }
    Master::master()->dead( m_fd );
    m_dead = true;
  }
}

Infotrope::Utils::magic_ptr<Context> const & Worker::context( std::string const & x ) const {
  Lock l__inst( m_cxt_lock );
#ifdef INFOTROPE_ENABLE_XPERSIST
  if( m_session ) {
    if( m_session->context_exists( x ) ) {
      return m_session->context( x );
    }
  }
#endif
  t_contexts::const_iterator i( m_contexts.find( x ) );
  if( i!=m_contexts.end() ) {
    return (*i).second;
  }
  throw std::runtime_error( "Context "+x+" does not exist in this session." );
}

bool Worker::context_exists( std::string const & x ) const {
  Lock l__inst( m_cxt_lock );
#ifdef INFOTROPE_ENABLE_XPERSIST
  if( m_session && m_session->context_exists( x ) ) {
    return true;
  }
#endif
  return m_contexts.find(x) != m_contexts.end();
}

void Worker::context_register( Utils::magic_ptr<Context> const & c ) {
  Lock l__inst( m_cxt_lock );
#ifdef INFOTROPE_ENABLE_XPERSIST
  if( m_session ) {
    if( c->persist() ) {
      m_session->context_register( c );
      return;
    }
  }
#endif
  m_contexts[c->name()] = c;
}

void Worker::context_finished( Utils::magic_ptr<Context> const & c ) {
  Lock l__inst( m_cxt_lock );
#ifdef INFOTROPE_ENABLE_XPERSIST
  if( m_session ) {
    if( m_session->context_exists( c->name() ) ) {
      m_session->context_finished( c );
    }
  }
#endif
  m_contexts.erase(c->name());
}

std::string const & Worker::login() const {
  if( m_login.empty() ) throw std::runtime_error( "Login requested from unauthenticated connection." );
  return m_login;
}

void Worker::login( std::string const & x ) {
  Master::master()->log( 5, "Login: " + x + " logged in" );
  m_login = x;
  const unsigned * n;
  if( SASL_OK == sasl_getprop( m_sasl_conn, SASL_SSF, reinterpret_cast<const void **>(&n) ) ) {
    if( (*n) ) {
      m_tcp->startsasl( m_sasl_conn );
      log( 5, "Negotiated SASL security layer." );
    }
  }
}

bool Worker::starttls( std::string const & cert_file ) {
  if( m_tcp->bits()!=0 ) {
    return false;
  }
  bool started( m_tcp->starttls( true, cert_file ) );
  if( started ) {
    unsigned long int tmp( m_tcp->bits() );
    sasl_setprop( m_sasl_conn, SASL_SSF_EXTERNAL, &tmp );
  }
  return started;
}

bool Worker::startdeflate() {
  return m_tcp->startdeflate();
}

#ifdef INFOTROPE_ENABLE_XPERSIST
void Worker::session_create() {
  if( !m_session ) {
    char buf[1024];
    gethostname( buf, 1024 );
    buf[1023] = 0;
    m_session = new Session( login() + "." + Data::Modtime::modtime().asString() + "@" + buf, *this );
    Master::master()->session( m_session );
    magic_ptr<Token::List> l( new Token::List );
    l->add( "*" );
    l->add( "SESSION" );
    l->add( m_session->name() );
    send( l );
  } else {
    throw std::runtime_error( "Session exists already!" );
  }
}

void Worker::session() {
  Infotrope::Threading::Lock l__inst( m_session_lock );
  if( !m_session ) {
    session_create();
  }
}

void Worker::set_session( std::string const & s ) {
  Infotrope::Threading::Lock l__inst( m_session_lock );
  if( m_session ) {
    throw std::runtime_error( "Already have session." );
  }
  m_session = Master::master()->session( s );
  if( m_session ) {
    if( m_session->login()!=login() ) {
      m_session.zap();
    }
  }
  if( !m_session ) {
    session_create();
  }
  m_session->bind( *this );
}

void Worker::set_session() {
  Infotrope::Threading::Lock l__inst( m_session_lock );
  session_create();
}
#endif

Worker::~Worker() {
#ifdef INFOTROPE_ENABLE_XPERSIST
  if( m_session ) {
    m_session->unbind();
    m_session.zap();
  }
#endif
  m_tcp.zap();
  close( m_fd );
  sasl_dispose( &m_sasl_conn );
}
