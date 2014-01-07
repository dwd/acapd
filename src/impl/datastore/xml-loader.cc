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
#include <infotrope/datastore/xml-loader.hh>
// #include <iostream>
#include <infotrope/datastore/constants.hh>
#include <infotrope/utils/base64.hh>
#include <infotrope/server/master.hh>

using namespace std;
using namespace Infotrope::XML;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope::Utils;
using namespace Infotrope::Server;

Loader::Loader()
  : Parser(), m_ds( Datastore::datastore() ), m_trans( false ),
    m_sane( false ) {
}

void Loader::start( std::string const & t, t_attrs_p const & a ) {
  Parser::start( t, a );
  //std::cout << "XML: Start " << t << std::endl;
  if ( t == "modify" ) {
    m_mode = ( *a ) [ "mode" ];
  } else if ( t == "dataset" ) {
    Path path( ( *a ) [ "name" ] );
    if ( !m_ds.exists( path ) ) {
      m_dataset = m_ds.create( path );
    } else {
      m_dataset = m_ds.dataset( path );
    }
    m_inh_reset = false;
    Master::master()->log( 1, "Loading dataset " + path.asString() );
  } else if ( t == "entry" ) {
    m_entry = new Entry();
  } else if ( t == "attribute" ) {
    m_attribute = new Attribute( ( *a ) [ "name" ] );
    m_attr_nil = false;
    m_acl_nil = false;
    m_attr_default = false;
    m_acl_default = false;
  } else if ( t == "modtime" ) {
    m_attribute = new Attribute( "modtime", ( *a ) [ "time" ] );
  } else if ( t == "multivalue" ) {
    m_value = new Token::PList;
  } else if ( t == "acl" ) {
    m_acl = new Acl;
  } else if ( t == "ace" ) {
    //Master::master()->log( 1, "Granting " + (*a)["identifier"] + " rights " + (*a)["rights"] );
    m_acl->grant( ( *a ) [ "identifier" ], ( *a ) [ "rights" ] );
    //Master::master()->log( 1, "Now have ACL ::" + m_acl->tokens()->asString() + "::" );
  } else if ( t == "subdataset" ) {
    // Need to see if we've already got one.
    Utils::magic_ptr<Token::Token> value( new Token::PList );
    if ( m_entry->exists( c_attr_subdataset ) ) {
      for ( int i( 0 );
	    i != m_entry->attr( c_attr_subdataset ) ->value()->toList().length();
	    ++i ) {
	value->toList().add( m_entry->attr( c_attr_subdataset ) ->value()->toList().ptr( i ) );
      }
    }
    value->toList().add( new Token::String( ( *a ) [ "href" ] ) );
    m_attribute = new Attribute( c_attr_subdataset, value );
    m_entry->add( m_attribute );
    m_attribute = 0;
  } else if( t=="nil" ) {
    t_tags::const_iterator i( m_tags.end() );
    --i;
    --i;
    //std::cout << "Got NIL. Last tag is " << (*i).tag << std::endl;
    if( (*i).tag=="attribute" ) {
      m_attr_nil = true;
    } else {
      m_acl_nil = true;
    }
  } else if( t=="default" ) {
    t_tags::const_iterator i( m_tags.end() );
    --i;
    --i;
    //std::cout << "Got DEFAULT. Last tag is " << (*i).tag << std::endl;
    if( (*i).tag=="attribute" ) {
      m_attr_default = true;
    } else {
      m_acl_default = true;
    }
  }
}

void Loader::text( std::string const & txt ) {
  //std::cout << "XML: text.\n" << std::flush;
  if ( current_tag().tag == "value" ) {
    //std::cout << "XML: Value tag.\n" << std::flush;
    std::string val( txt );
    //std::cout << "XML: Here.\n" << std::flush;
    if( current_tag().attrs ) {
      //std::cout << "XML: Have attrs.\n" << std::flush;
      t_attrs::const_iterator i( current_tag().attrs->find( "encoding" ) );
      if( i!=current_tag().attrs->end() ) {
	if( (*i).second=="base64" ) {
	  //std::cout << "XML: Decoding base64 attribute value.\n" << std::flush;
	  val = Base64::decode( txt );
	  //std::cout << "XML: Done base64.\n" << std::flush;
	}
      }
    }
    if ( !m_value ) {
      m_value = new Token::String( val );
      return ;
    } else if ( m_value->isString() ) {
      Utils::magic_ptr<Token::Token> p( new Token::PList );
      p->toList().add( m_value );
      m_value = p;
    }
    m_value->toList().add( new Token::String( val ) );
  }
  //std::cout << "XML: text end.\n" << std::flush;
}

void Loader::end( std::string const & tag ) {
  //std::cout << "XML: end " << tag << std::endl;
  if ( tag == "entry" ) {
    // Save entry.
    if( (*(current_tag().attrs))["name"] == "" ) {
      m_inh_reset = true;
    }
    m_dataset->add2( m_entry, (*(current_tag().attrs))["name"], ( m_mode == "replace" ) );
    m_entry = 0;
  } else if ( tag == "dataset" ) {
    // Now in a different dataset, or possibly no dataset at all.
    // We should hunt for the superior dataset tag here, but we don't
    // actually use this syntax in dumps.
    if( m_inh_reset ) {
      //Master::master()->log( 1, "Loader resetting inheritance" );
      m_dataset->inherit_reset();
      m_inh_reset = false;
    }
    Transaction::add( m_dataset->path() );
    Transaction::add( m_dataset->subcontext_pure( true ) );
  } else if ( tag == "attribute" ) {
    // Save attribute.
    //std::cout << "XML: Saving attribute.\n" << std::flush;
    if ( m_value ) {
      //std::cout << "XML: Saving value.\n" << std::flush;
      if( m_attribute->attribute_rep() == c_attr_dataset_acl ) {
	//Master::master()->log( 1, "Saving dataset.acl" );
	Utils::magic_ptr<Acl> acl( new Acl( m_value ) );
	acl->name( m_dataset->path().asString() );
	m_attribute->value_acl( acl );
      } else if( m_attribute->attribute().find( "dataset.acl" ) == 0 ) {
	//Master::master()->log( 1, "Saving " + m_attribute->attribute() );
	Utils::magic_ptr<Acl> acl( new Acl( m_value ) );
	acl->name( m_dataset->path().asString(), m_attribute->attribute().substr( sizeof("dataset.acl.") ) );
	m_attribute->value_acl( acl );
      } else {
	m_attribute->value( m_value );
      }
      m_value = 0;
    }
    //std::cout << "XML: Saving attribute.\n" << std::flush;
    m_entry->add( m_attribute );
    //std::cout << "XML: Saved.\n" << std::flush;
    if( m_attr_nil ) {
      //std::cout << "Storing NIL to attribute.\n" << std::flush;
      m_entry->store_nil( m_attribute->attribute() );
    } else if( m_attr_default ) {
      //std::cout << "Storing DEFAULT to attribute.\n" << std::flush;
      m_entry->store_default( m_attribute->attribute() );
    }
    if( m_acl_nil ) {
      //std::cout << "Storing NIL to acl.\n" << std::flush;
      m_entry->store_acl_nil( m_attribute->attribute() );
    } else if( m_acl_default ) {
      //std::cout << "Storing DEFAULT to acl..\n" << std::flush;
      m_entry->store_acl_default( m_attribute->attribute() );
    }
    m_attribute = 0;
    //std::cout << "All done.\n" << std::flush;
  } else if ( tag == "acl" ) {
    m_attribute->acl( m_acl );
    m_acl = 0;
  } else if ( tag == "acapservers" ) {
    // Last tag, so we commit.
    m_trans.commit();
    m_sane = true;
  } else if ( tag == "value" ) {
    if ( !m_value ) {
      m_value = new Token::String( "" );
    }
  }
  Parser::end( tag );
}
