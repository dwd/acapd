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
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/constants.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope;

namespace {
  
  class MyRights : public Command {
  public:
    MyRights( Worker & w )
      : Command( false, w ) {}
    
    void internal_parse( bool complete ) {
      if( m_toks->length() < 3 ) {
	throw std::runtime_error( "Need argument of ACL Object." );
      }
      if( !m_toks->get( 2 ).isPList() ) {
	throw std::runtime_error( "ACL Object needs to be list." );
      }
      if( !m_toks->get( 2 ).toList().length() ) {
	throw std::runtime_error( "Need dataset path at minimum." );
      }
      if( m_toks->get( 2 ).toList().length() > 3 ) {
	throw std::runtime_error( "ACL Object is 3 long at max." );
      }
      for( int i( 0 ); i!=m_toks->get( 2 ).toList().length(); ++i ) {
	if( m_toks->get( 2 ).toList().get( i ).isLiteralMarker() ) {
	} else if( m_toks->get( 2 ).toList().get( i ).isString() ) {
	  if( i==0 ) {
	    Path p( m_toks->get( 2 ).toList().get( i ).toString().value() );
	  }
	} else {
	  throw std::runtime_error( "String required." );
	}
      }
    }
    
    void main() {
      Datastore const & ds( Datastore::datastore_read() );
      Threading::ReadLock l__inst( ds.lock() );
      // It would be nice to use the already-existent myrights stuff.
      // Except we can't... *sigh*.
      Path dset_name( m_toks->get( 2 ).toList().get( 0 ).toString().value(), m_worker.login() );
      Utils::magic_ptr<Dataset> dset( ds.dataset( dset_name ) );
      std::string owner( "$owner" );
      if ( dset->path().has_owner() ) {
	owner = dset->path().owner().get();
      }
      
      Utils::magic_ptr<Entry> empty( dset->fetch2( c_entry_empty ) );
      switch ( m_toks->get( 2 ).toList().length() ) {
      default:
	throw std::runtime_error( "ACL Object expected." );
      case 3:
	{
	  Utils::magic_ptr < Token::List > l( new Token::List );
	  l->add( m_toks->ptr( 0 ) );
	  l->add( new Token::Atom( "MYRIGHTS" ) );
	  l->add( new Token::QuotedString( dset->myrights( m_toks->get( 2 ).toList().get( 2 ).toString().value(), m_toks->get( 2 ).toList().get( 1 ).toString().value(), m_worker.login() ).asString() ) );
	  m_worker.send( l, false );
	}
	break;
	
      case 2:
	{
	  if( empty->exists( "dataset.acl." + m_toks->get( 2 ).toList().get( 1 ).toString().value() ) ) {
	    Acl acl( empty->attr( "dataset.acl." + m_toks->get( 2 ).toList().get( 1 ).toString().value() )->value() );
	    Utils::magic_ptr < Token::List > l( new Token::List );
	    l->add( m_toks->ptr( 0 ) );
	    l->add( new Token::Atom( "MYRIGHTS" ) );
	    l->add( new Token::QuotedString( acl.myrights( m_worker.login(), owner ).asString() ) );
	    m_worker.send( l, false );
	    break;
	  }
	}
      case 1:
	{
	  Acl acl( empty->attr( c_attr_dataset_acl )->value() );
	  Utils::magic_ptr < Token::List > l( new Token::List );
	  l->add( m_toks->ptr( 0 ) );
	  l->add( new Token::Atom( "MYRIGHTS" ) );
	  l->add( new Token::QuotedString( acl.myrights( m_worker.login(), owner ).asString() ) );
	  m_worker.send( l, false );
	  break;
	}
      }
      {
	Utils::magic_ptr<Token::List> l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "OK" ) );
	l->add( new Token::QuotedString( "MyRights complete." ) );
	m_worker.send( l, true );
      }
    }
  };
  
  Command::Register<MyRights> f( "MYRIGHTS", AUTH );
}
