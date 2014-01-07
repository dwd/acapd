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
#include <infotrope/server/return.hh>
#include <infotrope/datastore/attribute.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/acl.hh>
#include <infotrope/datastore/exceptions.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Data;
using namespace Infotrope;
using namespace std;
using namespace Infotrope::Utils;


magic_ptr<Token::Token> Return::fetch_attr_metadata( Dataset const & ds, std::string const & md, Utils::magic_ptr<Attribute> const & a, StringRep::entry_type const & entry ) const {
  RightList rl;
  if( a->myrights() ) {
    rl = a->myrights().get();
  } else {
    rl = ds.myrights( entry, a->attribute(), m_login );
  }
  if ( md == "value" ) {
    if ( rl.have_right( 'r' ) ) {
      if( rl.have_right( '0' ) && a->value()->isString() ) {
	return magic_ptr<Token::Token>( new Token::LiteralString( a->valuestr() ) );
      } else {
	return a->value();
      }
    } else {
      return magic_ptr<Token::Token>( new Token::String() );
    }
  } else if ( md == "size" ) {
    if ( rl.have_right( 'r' ) ) {
      return a->size();
    } else {
      return magic_ptr<Token::Token>( new Token::String() );
    }
  } else if ( md == "attribute" ) {
    return magic_ptr<Token::Token>( new Token::String( a->attribute() ) );
  } else if ( md == "myrights" ) {
    if ( rl.have_right( 'r' ) ) {
      return magic_ptr<Token::Token>( new Token::String( rl.asString() ) );
    } else {
      return magic_ptr<Token::Token>( new Token::String() );
    }
  } else if ( md == "acl" ) {
    if ( rl.have_right( 'r' ) ) {
      return a->acl()->tokens();
    } else {
      return magic_ptr<Token::Token>( new Token::String() );
    }
  } else if( md == "activeacl" ) {
    if( rl.have_right( 'r' ) ) {
      return a->active_acl()->tokens();
    } else {
      return magic_ptr<Token::Token>();
    }
  } else if( md == "activeaclname" ) {
    if( rl.have_right( 'r' ) ) {
      return a->active_acl()->name();
    } else {
      return magic_ptr<Token::Token>();
    }
  } else if( md == "origin" ) {
    if( rl.have_right( 'r' ) ) {
      return magic_ptr<Token::Token>( new Token::String( a->origin().asString() ) );
    } else {
      return Utils::magic_ptr<Token::Token>();
    }
  } else {
    throw std::runtime_error( "Unknown metadata asked for in RETURN." );
  }
}

magic_ptr<Token::Token> Return::fetch_attr_metadata( Dataset const & ds, std::list < std::string > const & md, Utils::magic_ptr < Attribute > const & a, StringRep::entry_type const & entry ) const {
  Token::List * l;
  if ( md.size() > 1 ) {
    l = new Token::PList;
  } else {
    l = new Token::List;
  }
  for ( std::list < std::string > ::const_iterator i( md.begin() );
	i != md.end(); ++i ) {
    l->add( fetch_attr_metadata( ds, *i, a, entry ) );
  }
  return magic_ptr<Token::Token>( l );
}

magic_ptr<Token::Token> Return::fetch_entry_metadata( Dataset const & ds, Utils::magic_ptr<Entry> const & e, std::pair< StringRep::entry_type, std::list<std::string> > const & ret ) const {
  std::string entry( e->attr( "entry" ) ->valuestr() );
  if ( ret.first->find( '*' ) != std::string::npos ) {
    bool vendor_infotrope( false );
    if( 0==ret.first->find( "vendor.infotrope.system." ) ) {
      vendor_infotrope = true;
    }
    std::string attr_prefix( ret.first->substr( 0, ret.first->find( '*' ) ) );
    Token::List * l( new Token::PList );
    for ( Entry::const_iterator i( e->begin() ); i != e->end(); ++i ) {
      if ( ( *i ).first->find( attr_prefix ) == 0 ) {
	if( vendor_infotrope || (*i).first->find( "vendor.infotrope.system." )!=0 ) {
	  RightList rl;
	  if( (*i).second->myrights() ) {
	    rl = (*i).second->myrights().get();
	  } else {
	    rl = ds.myrights( entry, (*i).second->attribute(), m_login );
	  }
	  if( rl.have_right( 'r' ) ) {
	    l->add( fetch_attr_metadata( ds, ret.second, ( *i ).second, entry ) );
	  }
	}
      }
    }
    return magic_ptr<Token::Token>( l );
  } else {
    if ( e->exists( ret.first ) ) {
      return fetch_attr_metadata( ds, ret.second, e->attr( ret.first ), entry );
    } else {
      return fetch_attr_metadata( ds, ret.second, Utils::magic_ptr<Attribute>( new Attribute( ret.first ) ), entry );
    }
  }
}

magic_ptr<Token::Token> Return::fetch_entry_metadata( Dataset const & ds, Utils::magic_ptr<Entry> const & e ) const {
  Token::List * l;
  l = new Token::List;
  for ( t_return::const_iterator i( m_return.begin() );
	i != m_return.end(); ++i ) {
    l->add( fetch_entry_metadata( ds, e, ( *i ) ) );
  }
  return magic_ptr<Token::Token>( l );
}

Return::Return( Token::Token const & t, Utils::StringRep::entry_type const & login )
  : m_return(), m_login( login ) {
  parse( t );
}

void Return::parse( Token::Token const & t ) {
  std::string last_attr;
  for ( int attr( 0 ); attr < t.toList().length(); ++attr ) {
    if ( t.toList().get( attr ).isString() ) {
      if ( !last_attr.empty() ) {
	// Default is "value", or else "attribute value".
	std::list < std::string > ret;
	if ( last_attr.find( '*' ) != std::string::npos ) {
	  ret.push_back( "attribute" );
	}
	ret.push_back( "value" );
	m_return.push_back( make_pair( last_attr, ret ) );
      }
      last_attr = t.toList().get( attr ).toString().value();
    } else {
      std::list < std::string > ret;
      for ( int r( 0 ); r < t.toList().get( attr ).toList().length(); ++r ) {
	ret.push_back( t.toList().get( attr ).toList().get( r ).toString().value() );
      }
      m_return.push_back( make_pair( last_attr, ret ) );
      last_attr = "";
    }
  }
  if ( !last_attr.empty() ) {
    // Default is "value", or else "attribute value".
    std::list < std::string > ret;
    if ( last_attr.find( '*' ) != std::string::npos ) {
      ret.push_back( "attribute" );
    }
    ret.push_back( "value" );
    m_return.push_back( make_pair( last_attr, ret ) );
  }
}
