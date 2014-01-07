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

/***************************************************************************
 *  Copyright  2003 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/

#include <infotrope/server/command.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/server/exceptions.hh>
#include <infotrope/server/master.hh>
#include <map>

using namespace Infotrope;
using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;

namespace {
  std::string flatten_string( std::string const & s ) {
    std::string ret;
    for( std::string::size_type i( 0 ); i!=s.length(); ++i ) {
      switch( s[i] ) {
      case '\n':
      case '\r':
	ret += "\\\\";
	break;
      default:
	ret += s[i];
      }
    }
    return ret;
  }
}

Command::Command( bool sync, Worker & worker )
  : Threading::Thread(), m_sync( sync ), m_dead( false ), m_feeding( false ), m_worker( worker ) {
  }

Command::~Command() {
  //Master::master()->log( 1, "Command being destructed." );
}

bool Command::parse( bool complete ) {
  try {
    //Master::master()->log( 1, "Parsing..." );
    m_toks = m_worker.current_line();
    //Master::master()->log( 1, "Parsing " + m_toks->asString() );
    internal_parse( complete );
    //Master::master()->log( 1, "Parse complete." );
    return true;
  } catch( Infotrope::Server::Exceptions::Exception & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr( 0 ) );
    l->add( e.failtok() );
    if( e.respcode() ) {
      l->add( magic_cast<Token::Token>( e.respcode() ) );
    }
    l->add( new QuotedString( flatten_string( e.what() ) ) );
    m_worker.send( l, true );
  } catch( std::string & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr(0) );
    l->add( "BAD" );
    l->add( new QuotedString( "STR :: " + flatten_string( e ) ) );
    m_worker.send( l, true );
  } catch( std::exception & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr( 0 ) );
    l->add( "BAD" );
    l->add( new QuotedString( flatten_string( e.what() ) ) );
    m_worker.send( l, true );
  } catch( ... ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr( 0 ) );
    l->add( "BAD" );
    l->add( new QuotedString( "Unknown exception, command terminated." ) );
    m_worker.send( l, true );
  }
  return false;
}

void * Command::runtime() {
  //Master::master()->log( 1, "Command runtime started." );
  try {
    //Master::master()->log( 1, "Executing " + m_toks->asString() );
    main();
    //Master::master()->log( 1, "Execution complete." );
  } catch( Infotrope::Server::Exceptions::Exception & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr( 0 ) );
    l->add( e.failtok() );
    if( e.respcode() ) {
      l->add( magic_cast<Token::Token>( e.respcode() ) );
    }
    l->add( new QuotedString( flatten_string( e.what() ) ) );
    m_worker.send( l, true );
  } catch( std::string & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr(0) );
    l->add( "BAD" );
    l->add( new QuotedString( "STR :: " + flatten_string( e ) ) );
    m_worker.send( l, true );
  } catch( std::exception & e ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr(0) );
    l->add( "NO" );
    l->add( new QuotedString( flatten_string( e.what() ) ) );
    m_worker.send( l, true );
  } catch( ... ) {
    magic_ptr<List> l( new List );
    l->add( m_toks->ptr(0) );
    l->add( "BAD" );
    l->add( new QuotedString( "Unknown exception, command terminated." ) );
    m_worker.send( l, true );
  }
  if( !m_feeding ) {
    m_worker.post_message( Worker::Terminated );
    m_dead = true;
  }
  return 0;
  //Master::master()->log( 1, "Command terminated." );
}

bool Command::feed() {
  bool chk( false );
  //Master::master()->log( 5, "Feeding, toks currently " + m_toks->asString() );
  try {
    try {
      if( !m_feeding ) {
	throw std::runtime_error( "Command fed, but no feed requested?" );
      }
      chk = feed_internal();
    } catch( Infotrope::Server::Exceptions::Exception & e ) {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( e.failtok() );
      if( e.respcode() ) {
	l->add( magic_cast<Token::Token>( e.respcode() ) );
      }
      l->add( new QuotedString( flatten_string( e.what() ) ) );
      m_worker.send( l, true );
    } catch( std::string & e ) {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr(0) );
      l->add( "BAD" );
      l->add( new QuotedString( "STR :: " + flatten_string( e ) ) );
      m_worker.send( l, true );
    } catch( std::exception & e ) {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr(0) );
      l->add( "NO" );
      l->add( new QuotedString( flatten_string( e.what() ) ) );
      m_worker.send( l, true );
    } catch( ... ) {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr(0) );
      l->add( "BAD" );
      l->add( new QuotedString( "Unknown exception, command terminated." ) );
      m_worker.send( l, true );
    }
  } catch(...) {
    Master::master()->log( 8, "Whoops, exception during exception processing. This would be a bad thing." );
    Master::master()->log( 8, m_toks->asString() );
    m_worker.post_message( Worker::Terminated );
    m_dead = true;
    throw;
  }
  if( !chk ) {
    m_worker.post_message( Worker::Terminated );
    m_dead = true;
  }
  m_feeding = chk;
  return chk;
}

void Command::feed_me() {
  if( !m_sync ) {
    throw std::runtime_error( "Command requested feeding, but not sync!" );
  }
  //Master::master()->log( 5, "Command requesting feeding." );
  m_feeding = true;
  m_worker.feed_me( this );
}

bool Command::feed_internal() {
  throw std::runtime_error( "Command fed, but no action to take?" );
  return false; // What actually happens, not that this line gets reached.
}

void Command::execute() {
  if( m_sync ) {
    runtime();
  } else {
    start();
  }
}

namespace {
  typedef std::map< Utils::istring, Command::RegisterBase * > t_cmd_map;
  
  typedef std::map< Infotrope::Server::Scope, t_cmd_map > t_scope_map;
  
  t_scope_map & s_scope_map() {
    static t_scope_map s;
    return s;
  }
}

Command::RegisterBase::RegisterBase( Utils::istring const & cmd, Scope s ) {
  s_scope_map()[s][cmd] = this;
}

Command * Command::create( Utils::istring const & command, Scope s, Worker & worker ) {
  t_scope_map::const_iterator i( s_scope_map().find( s ) );
  if( i==s_scope_map().end() ) {
    return 0;
  }
  t_cmd_map::const_iterator j( (*i).second.find( command ) );
  if( j==(*i).second.end() ) {
    return 0;
  }
  return (*j).second->create( worker );
}

namespace Infotrope {
  namespace Server {
    namespace Exceptions {
      namespace {
	std::string const c_bad( "BAD" );
	std::string const c_no( "NO" );
      }
      
      Exception::Exception( Infotrope::Utils::magic_ptr<Infotrope::Token::PList> const & e,
			    std::string const & s,
			    std::string const & f )
	: runtime_error( s ), m_respcode( e ), m_failtok( f ) {}
      Exception::Exception( std::string const & s, std::string const & f )
	: runtime_error( s ), m_failtok( f ) {}
      
      bad::bad( std::exception const & e )
	: Exception( magic_ptr < Token::PList > (), e.what(), c_bad ) {}
      
      bad::bad( std::string const & e )
	: Exception( magic_ptr < Token::PList > (), e, c_bad ) {}
      
      bad::bad( magic_ptr < Token::PList > const & e, std::string const & s )
	: Exception( e, s, c_bad ) {}
      
      no::no( std::string const & e )
	: Exception( magic_ptr < Token::PList > (), e, c_no ) {}
      
      no::no( magic_ptr < Token::PList > const & e, std::string const & s )
	: Exception( e, s, c_no ) {}
      
      
    }
  }
}
