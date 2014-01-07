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
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/server/exceptions.hh>

#include <infotrope/server/master.hh>

using namespace Infotrope;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope::Server;

namespace {
  
  class Store : public Server::Command {
  public:
    Store( Server::Worker & w )
      : Server::Command( true, w ) {
    }
    
    void internal_parse( bool complete ) {
      if( m_toks->length() > 2 ) {
	for( int i(2); i<m_toks->length(); ++i ) {
	  if( m_toks->get(i).isList() ) {
	    if( complete || (i+1) < m_toks->length() ) {
	      if( m_toks->get( i ).toList().length() < 2 ) {
		throw std::runtime_error( std::string( "Store lists must have entry path, modifiers, then pairs of attribute, metadata." ) );
	      }
	      int foo( 1 );
	      for( ; foo < m_toks->get( i ).toList().length() && m_toks->get( i ).toList().get( foo ).isAtom(); ++foo );
	      if( 0 != ( ( m_toks->get(i).toList().length() - foo ) % 2 ) ) {
		throw std::runtime_error( std::string("Store lists must have entry path, modifiers, then pairs of attribute, value. ") + (complete?"[Complete]":"[Incomplete]") );
	      }
	    }
	    if( m_toks->get(i).toList().length()==0 ) {
	      throw std::runtime_error( "store-lists cannot be empty." );
	    } else if( m_toks->get(i).toList().get(0).isString() ) {
	      Path( m_toks->get(i).toList().get(0).toString().value() );
	    } else if( !m_toks->get(i).toList().get(0).isLiteralMarker() ) {
	      throw std::runtime_error( "store-list path must be a string." );
	    }
	  } else {
	    throw std::runtime_error( "Store takes one or more store-lists, which are lists." );
	  }
	}
      } else if( complete ) {
	throw std::runtime_error( "STORE Without store-lists is called NOOP..." );
      }
    }
    
    void main() {
      Datastore & ds( Datastore::datastore() );
      // Should have:
      // TAG STORE 1*store-list
      Transaction trans;
      std::list< Utils::magic_ptr<Token::List> > pending_send;
      for ( int i( 2 ); i != m_toks->length(); ++i ) {
	// store-list is:
	// entry-path - we want to turn this into a dataset path, and hand the rest to Dataset::store().
	Utils::magic_ptr<Token::List> storelist( Utils::magic_cast<Token::List>( m_toks->ptr( i ) ) );
	Token::List newstore( false );
	std::list<Utils::StringRep::entry_type> defaults;
	std::string dataset( storelist->get( 0 ).toString().value() );
	std::string entry( dataset.substr( dataset.find_last_of( '/' ) + 1 ) );
	dataset = dataset.substr( 0, dataset.find_last_of( '/' ) + 1 );
	Path path( dataset, m_worker.login() );
	Utils::magic_ptr<Dataset> dset;
	if ( ds.exists( path ) ) {
	  dset = ds.dataset( path );
	}
	for ( int i( 1 ); i < storelist->length(); ++i ) {
	  if ( storelist->get( i ).isAtom() ) {
	    if ( storelist->get( i ).toAtom().value() == "NOCREATE" ) {
	      if ( !dset ) {
		Utils::magic_ptr<Token::PList> l( new Token::PList );
		l->add( new Token::Atom( "NOEXIST" ) );
		l->add( new Token::String( path.asString() ) );
		throw Server::Exceptions::no( l, "NOCREATE in store-list and dataset does not exist." );
	      }
	    } else if ( storelist->get( i ).toAtom().value() == "UNCHANGEDSINCE" ) {
	      ++i;
	      if ( dset ) {
		Utils::magic_ptr<Entry> ne( dset->fetch2( entry ) );
		if( ne ) {
		  if ( Modtime( ne->attr( c_attr_modtime )->value()->toString().value() ) > Modtime( storelist->get( i ).toString().value() ) ) {
		    Utils::magic_ptr<Token::PList> l( new Token::PList );
		    l->add( new Token::Atom( "MODIFIED" ) );
		    l->add( storelist->ptr( 0 ) );
		    throw Server::Exceptions::no( l, "Entry modified at " + ne->attr( c_attr_modtime )->value()->toString().value() );
		  }
		}
	      }
#ifdef INFOTROPE_ENABLE_XUNIQUE
	    } else if ( storelist->get( i ).toAtom().value() == "XUNIQUE"
			|| storelist->get( i ).toAtom().value() == "UNIQUE" ) {
	      std::string entry_save( entry );
	      entry += Modtime::modtime().asString();
	      if( dset ) {
		while( dset->exists2( entry ) ) entry = entry_save + Modtime::modtime().asString();
	      }
	      defaults.push_back( "entry" );
#endif
	    } else {
	      throw Server::Exceptions::bad( "Unknown store-list modifier "+storelist->get( i ).asString() );
	    }
	  } else {
	    newstore.add( new Token::String( entry ) );
	    for ( ; i < storelist->length(); i+=2 ) {
	      if( storelist->get(i).toString().value().find_first_of( "%*" )!=std::string::npos ) {
		throw Server::Exceptions::bad( "Attribute contains invalid characters." );
	      }
	      newstore.add( storelist->ptr( i ) );
	      if( storelist->get(i+1).isList() ) {
		newstore.add( storelist->ptr( i+1 ) );
	      } else {
		Token::PList * l( new Token::PList( false ) );
		l->add( new Token::String( "value" ) );
		l->add( storelist->ptr( i+1 ) );
		newstore.add( l );
	      }
	      for( int n( 0 ); n<newstore.get( newstore.length()-1 ).toList().length(); n+=2 ) {
		if( newstore.get( newstore.length()-1 ).toList().get( n ).toString().value() == "value" ) {
		  if( newstore.get( newstore.length()-1 ).toList().get( n+1 ).isAtom() ) {
		    if( !newstore.get( newstore.length()-1 ).toList().get( n+1 ).toAtom().isNil() ) {
		      if( newstore.get( newstore.length()-1 ).toList().get( n+1 ).toAtom().value()=="DEFAULT" ) {
			defaults.push_back( newstore.get( newstore.length()-2 ).toString().value() );
		      }
		    }
		  }
		}
	      }
	    }
	    break;
	  }
	}
	if ( !dset ) {
	  dset = ds.create( path );
	  if ( !dset->myrights( "", "entry", m_worker.login() ).have_rights( RightList( "ri" ) ) ) {
	    if ( path.has_owner() ) {
	      throw Server::Exceptions::no( "Cannot create dataset." );
	    }
	    // Allow it if there's no owner, any attempt to actually store
	    // will probably trip up later and blow the transaction.
	  }
	  
	  
	} else if( !dset->myrights( "", "entry", m_worker.login() ).have_rights( RightList( "rw" ) ) ) {
	  if( path.has_owner() ) {
	    throw Server::Exceptions::no( "Cannot store to dataset." );
	  }
	}
	Master::master()->log( 1, "Processing SL " + newstore.asString() + " for " + m_worker.login() );
	dset->store( Utils::magic_ptr<Token::List>( newstore ), m_worker.login() );
	dset.zap();
	if( ds.exists( path ) ) {
	  dset = ds.dataset( path );
	}
	Master::master()->log( 1, "Okay, completed SL " + newstore.asString() );
	if( !defaults.empty() ) {
	  Master::master()->log( 1, "Scanning defaults." );
	  Utils::magic_ptr<Token::List> l( new Token::List );
	  l->add( m_toks->ptr(0) );
	  l->add( new Token::Atom( "ENTRY" ) );
	  l->add( storelist->ptr( 0 ) );
	  Master::master()->log( 1, "Scanning now." );
	  for( std::list<Utils::StringRep::entry_type>::const_iterator i( defaults.begin() ); i!=defaults.end(); ++i ) {
	    Utils::magic_ptr<Token::Token> tok( new Token::PList );
	    tok->toList().add( new Token::String( (*i).get() ) );
	    Master::master()->log( 1, "Checking default for " + (*i).get() );
	    bool dump_nil( true );
	    if( dset ) {
	      Master::master()->log( 1, "Dataset still exists." );
	      Utils::magic_ptr<Entry> e( dset->fetch2( entry ) );
	      if( e ) {
		Master::master()->log( 1, "So does the entry." );
		if( e->exists( (*i) ) ) {
		  Master::master()->log( 1, "And the attribute." );
		  if( dset->myrights( entry, c_attr_entry, m_worker.login() ).have_right( 'r' ) ) {
		    Master::master()->log( 1, "Allowed to read entry." );
		    if( dset->myrights( entry, (*i), m_worker.login() ).have_right( 'r' ) ) {
		      Master::master()->log( 1, "Allowed to read attribute." );
		      Utils::magic_ptr<Attribute> x( e->attr( (*i) ) );
		      if( x ) {
			Master::master()->log( 1, "Attribute not shot." );
			if( x->value() ) {
			  Master::master()->log( 1, "Attribute value pointer not shot either." );
			  Master::master()->log( 1, x->value()->asString() );
			  tok->toList().add( x->value() );
			  dump_nil = false;
			  Master::master()->log( 1, "Added." );
			}
		      }
		    }
		  }
		}
	      }
	    }
	    if( dump_nil ) {
	      tok->toList().add( Utils::magic_ptr<Token::Token>() );
	    }
	    Master::master()->log( 1, "Adding token." );
	    l->add( tok );
	    Master::master()->log( 1, "Nexti" );
	  }
	  Master::master()->log( 1, "Sending defaults" );
	  m_worker.send( l );
	}
      }
      try {
	trans.commit();
      } catch( std::exception & e ) {
	Utils::magic_ptr<Token::List> l( new Token::List );
	l->add( "*" );
	l->add( "ALERT" );
	l->add( new Token::QuotedString( e.what() ) );
	m_worker.send( l );
      }
      {
	Utils::magic_ptr<Token::List> l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "OK" ) );
	l->add( new Token::String( "Store complete." ) );
	m_worker.send( l, true );
      }
    }
  };
  
  Server::Command::Register<Store> f( "STORE", Server::AUTH );
}
