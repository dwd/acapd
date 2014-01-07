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
 *  Thu Feb 13 12:53:37 2003
 *  Copyright  2003 Dave Cridland
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
  
  class DeleteAcl : public Command {
  public:
    DeleteAcl( Worker & w )
      : Command( false, w ) {
    }
    
    void aclmangle() {
      Datastore const & ds( Datastore::datastore() );
      Transaction trans;
      if ( m_toks->length() < 3 ) {
	throw Server::Exceptions::bad( "Need ACL objects." );
      }
      Path ndset( m_toks->get( 2 ).toList().get( 0 ).toString().value(), m_worker.login() );
      Utils::magic_ptr < Dataset > dset( ds.dataset( ndset ) );
      // Should have an ACL object, optionally a authid
      // We want to turn this into a STORE. Therefore:
      // 1) We need to figure out whether we're dealing with an existent ACL,
      // or actually an attribute value.
      Utils::magic_ptr<Entry> empty( dset->fetch2( c_entry_empty ) );
      if ( m_toks->get( 2 ).toList().length() == 3 ) {
	// ACL metadata on attribute.
	
	Utils::magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->get( 2 ).toList().ptr( 2 ) );
	l->add( m_toks->get( 2 ).toList().ptr( 1 ) );   // Copy attribute name.
	Utils::magic_ptr < Token::PList > pl( new Token::PList );
	pl->add( new Token::String( "acl" ) );
	if ( m_toks->length() == 3 ) {
	  pl->add( new Token::Atom );   // NIL
	}
	// Extract old ACL, if any
	
	
	Utils::magic_ptr<Acl> acl;
	Utils::magic_ptr<Entry> e( dset->fetch2( m_toks->get( 2 ).toList().get( 2 ).toString().value() ) );
	if ( e ) {
	  if ( e->exists( m_toks->get( 2 ).toList().get( 1 ).toString().value() ) ) {
	    Utils::magic_ptr < Attribute > a( e->attr( m_toks->get( 2 ).toList().get( 1 ).toString().value() ) );
	    if ( a->acl().ptr() ) {
	      // Ah, good. Copy it/
	      acl = Utils::magic_ptr < Acl > ( new Acl( a->acl().get() ) );
	    }
	  }
	}
	if ( acl.ptr() ) {
	  if ( m_toks->length() > 3 ) {
	    if ( acl->have_authid( m_toks->get( 3 ).toString().value() ) ) {
	      acl->remove( m_toks->get( 3 ).toString().value() );
	    } else {
	      return ;   // No need to remove it, it's not there.
	    }
	    
	    
	    pl->add( acl->tokens() );
	    l->add( Utils::magic_cast < Token::Token > ( pl ) );
	    dset->store( l, m_worker.login() );
	  }
	} else {
	  return ;
	}
      } else {
	// We're changing an attribute.
	Utils::magic_ptr < Token::List > l( new Token::List );
	l->add( new Token::String( "" ) );
	std::string attrname;
	if ( m_toks->get( 2 ).toList().length() == 1 ) {
	  attrname = c_attr_dataset_acl.get();
	} else {
	  attrname = "dataset.acl." + m_toks->get( 2 ).toList().get( 1 ).toString().value();
	}
	l->add( new Token::String( attrname ) );
	Utils::StringRep::entry_type attr( attrname );   // Look up once.
	Utils::magic_ptr < Acl > acl;
	if ( empty->exists( attr ) ) {
	  if ( empty->attr( attr ) ->value().ptr() ) {
	    acl = Utils::magic_ptr < Acl > ( new Acl( empty->attr( attr ) ->value() ) );
	  }
	}
	Utils::magic_ptr < Token::PList > pl( new Token::PList );
	pl->add( new Token::String( "value" ) );
	if ( m_toks->length() == 3 ) {
	  pl->add( new Token::Atom );   // NIL
	}
	
	
	if ( acl.ptr() ) {
	  if ( m_toks->length() > 3 ) {
	    if ( acl->have_authid( m_toks->get( 3 ).toString().value() ) ) {
	      acl->remove( m_toks->get( 3 ).toString().value() );
	    } else {
	      return ;   // Not there, no point mucking about.
	    }
	    
	    
	    pl->add( acl->tokens() );
	  }
	  l->add( magic_cast < Token::Token > ( pl ) );
	  dset->store( l, m_worker.login() );
	} else {
	  return ;
	}
      }
      trans.commit();
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
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
      aclmangle();
      {
	magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "OK" ) );
	l->add( new Token::QuotedString( "ACL Object changed." ) );
	m_worker.send( l, true );
      }
    }
  };
  
  Command::Register<DeleteAcl> f( "DELETEACL", AUTH );
}
