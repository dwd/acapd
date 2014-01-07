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
 *  Copyright  2004 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/


#include <infotrope/server/command.hh>
#include <infotrope/server/exceptions.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/datastore/constants.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Data;
using namespace Infotrope::Utils;
using namespace Infotrope::Constants;
using namespace Infotrope;

namespace {
  
  class ListRights : public Command {
  public:
    ListRights( Worker & w )
      : Command( false, w ) {}
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 4 ) {
	throw std::runtime_error( "Too few arguments - need ACL Object" );
      }
      if( m_toks->length() > 4 ) {
	throw std::runtime_error( "Too many arguments" );
      }
      if( m_toks->length() > 2 ) {
	if( m_toks->get( 2 ).isList() ) {
	  for( int i(0); i!=m_toks->get( 2 ).toList().length(); ++i ) {
	    if( i > 2 ) {
	      throw std::runtime_error( "ACL Object too long." );
	    }
	    if( m_toks->get( 2 ).toList().get( i ).isString() ) {
	      if( i==0 ) {
		Path p( m_toks->get( 2 ).toList().get( i ).toString().value() );
	      }
	    } else if( !m_toks->get( 2 ).toList().get( i ).isLiteralMarker() ) {
	      throw std::runtime_error( "ACL Object should only contain strings." );
	    }
	  }
	} else {
	  throw std::runtime_error( "ACL Object should be list of strings." );
	}
      }
      if( m_toks->length() > 3 ) {
	if( !m_toks->get( 3 ).isString() && !m_toks->get( 3 ).isLiteralMarker() ){
	  throw std::runtime_error( "Authzid should be string." );
	}
      }
    }
    
    void main() {
      Datastore const & ds( Datastore::datastore_read() );
      Threading::ReadLock l__inst( ds.lock() );
      if ( m_toks->length() != 4 ) {
	throw Server::Exceptions::bad( "Need ACL Object." );
      }
      bool have_admin( false );
      {
	Path dset_name( m_toks->get( 2 ).toList().get( 0 ).toString().value(), m_worker.login() );
	Utils::magic_ptr < Dataset > dset( ds.dataset( dset_name ) );
	std::string owner( "$owner" );
	if ( dset->path().has_owner() ) {
	  owner = dset->path().owner().get();
	}
	Utils::magic_ptr<Entry> empty( dset->fetch2( c_entry_empty ) );
	
	switch ( m_toks->get( 2 ).toList().length() ) {
	default:
	  throw Server::Exceptions::bad( "ACL Object expected." );
	case 3:
	  {
	    have_admin = dset->myrights( m_toks->get( 2 ).toList().get( 2 ).toString().value(), m_toks->get( 2 ).toList().get( 1 ).toString().value(), m_worker.login() ).have_right( 'a' );
	  }
	  break;
	  
	case 2:
	  if ( empty->exists( "dataset.acl." + m_toks->get( 2 ).toList().get( 1 ).toString().value() ) ) {
	    Acl acl( empty->attr( "dataset.acl." + m_toks->get( 2 ).toList().get( 1 ).toString().value() ) ->value() );
	    have_admin = acl.myrights( m_worker.login(), owner ).have_right( 'a' );
	    break;
	  }
	case 1:
	  {
	    Acl acl( empty->attr( c_attr_dataset_acl ) ->value() );
	    have_admin = acl.myrights( m_worker.login(), owner ).have_right( 'a' );
	    break;
	  }
	}
      }
      {
	magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "LISTRIGHTS" ) );
	l->add( new Token::String( "" ) );
	if ( have_admin ) {
	  l->add( new Token::String( "r" ) );
	  l->add( new Token::String( "w" ) );
	  l->add( new Token::String( "i" ) );
	  l->add( new Token::String( "a" ) );
	  l->add( new Token::String( "x" ) );
	  l->add( new Token::String( "0" ) );
	}
	m_worker.send( l, false );
      }
      {
	magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "OK" ) );
	l->add( new Token::String( "LISTLEFTS completed." ) );
	m_worker.send( l, true );
      }
    }
  };
  
  Command::Register<ListRights> f( "LISTRIGHTS", AUTH );
}
