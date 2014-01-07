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
 *  Thu Feb 13 12:50:48 2003
 *  Copyright  2004  dwd
 *  dwd@turner.gestalt.entity.net
 ****************************************************************************/

#include <infotrope/server/command.hh>
#include <infotrope/server/master.hh>
#include <infotrope/server/exceptions.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/threading/rw-lock.hh>

extern "C" {
#include <sasl/sasl.h>
}

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Data;
using namespace Infotrope;

namespace {
  
  class Authenticate : public Command {
  private:
    std::string m_username;
    std::string m_password;
  public:
    Authenticate( Worker & w )
      : Command( true, w ) { // Authenticate is synchronous.
    }
    
    void extract_plain( std::string const & srx ) {
      std::string::size_type first_nul( srx.find( '\0' ) );
      std::string::size_type second_nul( srx.find( '\0', first_nul+1 ) );
      m_username = srx.substr( first_nul + 1, second_nul - 1 );
      m_password = srx.substr( second_nul + 1 );
    }
    
    // tag AUTHENTICATE <str mechanism> [<str CSF>]
    // Min length 3, max 4.
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Need mechanism name." );
      }
      if( m_toks->length() > 4 ) {
	throw std::runtime_error( "Too many arguments." );
      }
      if( m_toks->length() > 2 ) {
	if( !m_toks->get(2).isString() && !m_toks->get(2).isLiteralMarker() ) {
	  m_toks->get(2).toString();
	  throw std::runtime_error( "Mechanism name must be a string." );
	}
	if( m_toks->length() > 3 ) {
	  if( !m_toks->get(3).isString() && !m_toks->get(3).isLiteralMarker() ) {
	    throw std::runtime_error( "Need string data for client-send-first." );
	  }
	}
      }
    }
    
    bool feed_internal() {
      //Master::master()->log( 5, "feed_internal " + m_toks->asString() );
      const char * out;
      unsigned outlen;
      int result;
      Utils::magic_ptr<Token::List> rx( m_worker.current_line() );
      if( m_toks->get( 2 ).toString().value()=="PLAIN" ) {
	extract_plain( rx->get( 0 ).toString().value() );
      }
      result = sasl_server_step( m_worker.sasl_conn(), rx->get( 0 ).toString().value().c_str(), rx->get( 0 ).toString().value().length(), &out, &outlen );
      //Master::master()->log( 1, std::string( "Current SASL result is " ) + sasl_errstring( result, NULL, NULL ) );
      return handle( result, out, outlen );
    }
    
    bool handle( int result, const char * out, unsigned outlen ) {
      //Master::master()->log( 5, "handle " + m_toks->asString() );
      if( result == SASL_CONTINUE ) {
	magic_ptr<Token::List> l( new Token::List );
	l->add( new Token::Atom( "+" ) );
	l->add( new Token::LiteralString( out ) );
	m_worker.send( l, true );
	return true;
      } else {
	//Master::master()->log( 5, std::string("SASL returned ") + sasl_errstring( result, NULL, NULL ) + ", meaning " + sasl_errdetail( m_worker.sasl_conn() ) );
	if( result == SASL_OK ) {
	  m_worker.pop_scope();
	  m_worker.push_scope( AUTH );
	  const char * user;
	  if( SASL_OK != sasl_getprop( m_worker.sasl_conn(), SASL_USERNAME, reinterpret_cast< const void ** >( &user ) ) ) {
	    Master::master()->log( 5, std::string("Failed to get user, error is ")+sasl_errdetail( m_worker.sasl_conn() ) );
	    user = "anonymous";
	  }
	  magic_ptr<Token::List> l( new Token::List );
	  l->add( m_toks->ptr( 0 ) );
	  l->add( new Token::Atom( "OK" ) );
	  if ( out ) {
	    magic_ptr<Token::List> pl( new Token::PList );
	    pl->add( new Token::Atom( "SASL" ) );
	    pl->add( new Token::QuotedString( out ) );
	    l->add( magic_cast<Token::Token>( pl ) );
	  }
	  l->add( new Token::String( std::string("You're logged in as ")+user+". Frooby." ) );
	  m_worker.send( l, true );
	  m_worker.login( user );
	  m_worker.login();
	  return false;
	} else if( result == SASL_TOOWEAK ) {
	  Utils::magic_ptr<Token::PList> l( new Token::PList );
	  l->add( new Token::Atom( "AUTH-TOO-WEAK" ) );
	  throw Server::Exceptions::no( l, sasl_errdetail( m_worker.sasl_conn() ) );
	} else if( result == SASL_ENCRYPT ) {
	  Utils::magic_ptr<Token::PList> l( new Token::PList );
	  l->add( new Token::Atom( "ENCRYPT-NEEDED" ) );
	  throw Server::Exceptions::no( l, sasl_errdetail( m_worker.sasl_conn() ) );
	} else if( result == SASL_TRANS ) {
	  Utils::magic_ptr<Token::PList> l( new Token::PList );
	  l->add( new Token::Atom( "TRANSITION-NEEDED" ) );
	  throw Server::Exceptions::no( l, sasl_errdetail( m_worker.sasl_conn() ) );
	} else if( result == SASL_NOUSER ) {
	  Datastore const & ds( Datastore::datastore_read() );
	  Threading::ReadLock l__inst( ds.lock() );
	  magic_ptr<Dataset> dset( ds.dataset( Path( "/vendor.infotrope.acapd/site/" ) ) );
	  magic_ptr<Entry> e( dset->fetch2( "auth_provision" ) );
	  if( !e || !e->exists( "vendor.infotrope.acapd.value" ) ) {
	    throw Server::Exceptions::no( sasl_errdetail( m_worker.sasl_conn() ) );
	  }
	  std::string const & val( e->attr( "vendor.infotrope.acapd.value" )->valuestr() );
	  if( val=="1" ) {
	    if( m_toks->get( 2 ).toString().value()=="PLAIN" ) {
	      // Provision account.
	      int ret = sasl_setpass( m_worker.sasl_conn(), m_username.c_str(), m_password.c_str(), m_password.length(), NULL, 0, SASL_SET_CREATE ); 
	      if( ret == SASL_OK ) {
		magic_ptr<Token::List> l( new Token::List );
		l->add( m_toks->ptr( 0 ) );
		l->add( new Token::Atom( "OK" ) );
		l->add( new Token::String( std::string("Account created as ")+m_username+". Frooby." ) );
		m_worker.send( l, true );
		m_worker.pop_scope();
		m_worker.push_scope( AUTH );
		m_worker.login( m_username );
		m_worker.login();
		return false;
	      } else {
		throw Server::Exceptions::no( sasl_errdetail( m_worker.sasl_conn() ) );
	      }
	    } else {
	      Utils::magic_ptr<Token::PList> l( new Token::PList );
	      l->add( new Token::Atom( "TRANSITION-NEEDED" ) );
	      throw Server::Exceptions::no( l, sasl_errdetail( m_worker.sasl_conn() ) );
	    }
	  }
	  throw Server::Exceptions::no( sasl_errdetail( m_worker.sasl_conn() ) );
	}
	throw Server::Exceptions::no( sasl_errdetail( m_worker.sasl_conn() ) );
      }
    }
    
    void main() {
      const char * out;
      unsigned outlen;
      int result;
      //Master::master()->log( 5, "main " + m_toks->asString() );
      Master::master()->log( 5, "Authentication attempt started with mechanism " + m_toks->get(2).toString().value() );
      if ( m_toks->length() == 4 ) {
	if( m_toks->get( 2 ).toString().value()=="PLAIN" ) {
	  extract_plain( m_toks->get( 3 ).toString().value() );
	}
	result = sasl_server_start( m_worker.sasl_conn(), m_toks->get( 2 ).toString().value().c_str(), m_toks->get( 3 ).toString().value().c_str(), m_toks->get( 3 ).toString().value().length(), &out, &outlen );
      } else {
	result = sasl_server_start( m_worker.sasl_conn(), m_toks->get( 2 ).toString().value().c_str(), NULL, 0, &out, &outlen );
      }
      if( handle( result, out, outlen ) ) {
	feed_me();
      }
      //Master::master()->log( 5, "main end " + m_toks->asString() );
    }
  };
  
  Command::Register<Authenticate> f( "AUTHENTICATE", PREAUTH );
}
