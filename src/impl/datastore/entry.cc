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
#include <infotrope/datastore/entry.hh>
//#include "acap-subcontext.hh"
#include <infotrope/datastore/modtime.hh>
#include <infotrope/datastore/constants.hh>

using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope::Utils;

Entry::t_attr Entry::s_noattr;

Entry::Entry()
  : m_attrs() {}



Entry::Entry( Utils::StringRep::entry_type const & entry )
  : m_attrs() {
  add( magic_ptr<Attribute>( new Attribute( c_attr_entry, magic_ptr<Token::Token>( new Token::String( entry.get() ) ) ) ) );
}

Entry::t_attr const & Entry::attr( Entry::t_attr_name const & n ) const {
  const_iterator i( m_attrs.find( n ) );
  if ( i == end() ) {
    if ( exists( c_attr_entry ) ) {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist in entry '" + attr( "entry" )->valuestr() + "'" );
    } else {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist." );
    }
  }
  return ( *i ).second;
}

bool Entry::attr_isnil( Entry::t_attr_name const & n ) const {
  const_iterator i( m_attrs.find( n ) );
  if ( i == end() ) {
    return false;
    /*
    if ( exists( c_attr_entry ) ) {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist in entry '" + attr( "entry" ) ->valuestr() + "'" );
    }
    else {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist." );
    }
    */
  }
  if ( ( *i ).second->value() && ( *i ).second->value()->isNil() ) {
    return true;
  }
  return false;
}

bool Entry::attr_isdefault( Entry::t_attr_name const & n ) const {
  const_iterator i( m_attrs.find( n ) );
  if ( i == end() ) {
    return true;
    /*
    if ( exists( c_attr_entry ) ) {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist in entry '" + attr( "entry" ) ->valuestr() + "'" );
    }
    else {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist." );
    }
    */
  }
  if ( !( *i ).second->value() ) {
    return true;
  }
  return false;
}

bool Entry::acl_isnil( Entry::t_attr_name const & n ) const {
  const_iterator i( m_attrs.find( n ) );
  if ( i == end() ) {
    return false;
    /*
    if ( exists( c_attr_entry ) ) {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist in entry '" + attr( "entry" ) ->valuestr() + "'" );
    }
    else {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist." );
    }
    */
  }
  if ( ( *i ).second->acl() && ( *i ).second->acl()->isNil() ) {
    return true;
  }
  return false;
}

bool Entry::acl_isdefault( Entry::t_attr_name const & n ) const {
  const_iterator i( m_attrs.find( n ) );
  if ( i == end() ) {
    return true;
    /*
    if ( exists( c_attr_entry ) ) {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist in entry '" + attr( "entry" ) ->valuestr() + "'" );
    }
    else {
      throw std::runtime_error( "Attribute " + n.get() + " doesn't exist." );
    }
    */
  }
  if ( !( *i ).second->acl() ) {
    return true;
  }
  return false;
}

void Entry::add( Entry::t_attr const & a ) {
  m_attrs[ a->attribute() ] = a;
}

void Entry::store_nil( t_attr_name const & name ) {
  if( !exists_pure( name ) ) {
    m_attrs[ name ] = Utils::magic_ptr<Attribute>( new Attribute( name ) );
  } else {
    m_attrs[ name ]->value( Utils::magic_ptr<Token::Token>( new Token::String() ) );
  }
}

void Entry::store_default( t_attr_name const & name ) {
  if( !exists_pure( name ) ) {
    m_attrs[ name ] = Utils::magic_ptr<Attribute>( new Attribute( name ) );
  }
  m_attrs[ name ]->value( Utils::magic_ptr<Token::Token>() );
}

void Entry::store_acl_nil( t_attr_name const & name ) {
  if( !exists_pure( name ) ) {
    m_attrs[ name ] = Utils::magic_ptr<Attribute>( new Attribute( name ) );
  } else {
    m_attrs[ name ]->acl( Acl( Utils::magic_ptr<Token::Token>() ) );
  }
}

void Entry::store_acl_default( t_attr_name const & name ) {
  if( !exists_pure( name ) ) {
    m_attrs[ name ] = Utils::magic_ptr<Attribute>( new Attribute( name ) );
  }
  m_attrs[ name ]->acl( Utils::magic_ptr<Acl>() );
}

void Entry::erase( t_attr_name const & name ) {
  t_attrs::iterator i( m_attrs.find( name ) );
  if ( i != m_attrs.end() ) {
    m_attrs.erase( i );
  }
}

// Merge - effectively copy the attributes from one entry to another,
// honouring DEFAULT/NIL.
// If inherit flag is set, then we will delete markers, otherwise, we'll
// leave them in place.
// If the inherit flag is not set
void Entry::merge( Utils::magic_ptr < Entry > const & e, bool inherit ) {
#if 0
  if( attr( c_attr_entry )->value()->isNil() ) {
    // We are deleted. Save this attribute, wipe everything else.
    Utils::magic_ptr<Attribute> entry( m_attrs[ c_attr_entry ] );
    Utils::magic_ptr<Attribute> dtime;
    if( exists( c_attr_entry_dtime ) ) {
      dtime = m_attrs[ c_attr_entry_dtime ];
    }
    m_attrs.clear();
    m_attrs[c_attr_entry] = entry;
    if( dtime ) {
      m_attrs[c_attr_entry_dtime] = entry;
    }
    if( attr_isnil( c_attr_entry ) ) {
      // We are formally NIL, so don't merge at all.
      return;
    }
  }
#endif
  // Copy across anything we don't have explicitly set.
  for ( const_iterator i( e->begin() ); i != e->end(); ++i ) {
    if ( inherit && ( 
		     ( *i ).first->find( "vendor.infotrope.system.noinherit" ) == 0
		     || (*i).first->find( "dataset.acl" ) == 0 ) ) {
      // Don't copy non-inheritable attributes.
    } else {
      if ( m_attrs.find( ( *i ).first ) == m_attrs.end() ) {
	// Ours doesn't exist, copy the lot.
	m_attrs[ ( *i ).first ] = ( *i ).second;
      } else {
	bool copy_val( attr_isdefault( (*i).first ) );
	bool copy_acl( acl_isdefault( (*i).first ) );
	
	if ( ( *i ).first == c_attr_modtime
	     || ( *i ).first == c_attr_dataset_inherit_modtime
	     || ( *i ).first == c_attr_entry_rtime ) {
	  // Pick the latest value, or whichever exists.
	  if ( !( m_attrs[ ( *i ).first ]->value()->isNil()
		  || ( *i ).second->value()->isNil() ) ) {
	    if ( Modtime( m_attrs[ ( *i ).first ]->valuestr() ) <
		 Modtime( ( *i ).second->valuestr() ) ) {
	      copy_val = false;
	    }
	  }
	}
	
	// Now handle copy.
	if( copy_val ) {
	  if( copy_acl ) {
	    m_attrs[ (*i).first ] = (*i).second;
	  } else {
	    Utils::magic_ptr<Attribute> a( (*i).second->clone() );
	    a->acl( m_attrs[(*i).first]->acl() );
	    m_attrs[ (*i).first ] = a;
	  }
	} else {
	  if( copy_acl ) {
	    Utils::magic_ptr<Attribute> a( (*i).second->clone() );
	    a->acl( m_attrs[(*i).first]->acl() );
	    m_attrs[ (*i).first ] = a;
	  }
	}
	
	// Erase markers.
	if ( !m_attrs[ ( *i ).first ]->value() && !m_attrs[ (*i).first ]->acl() ) {
	  if( !(*i).first->empty() ) {
	    erase( (*i).first );
	  }
	}
      }
    }
  }
}

// Clone the entry. Note that Attributes within the entry are cloned, too, so
// changing these will not change attributes in the original.
Infotrope::Utils::magic_ptr<Entry> Entry::clone( bool inherit ) const {
  Infotrope::Utils::magic_ptr<Entry> r( new Entry() );
  r->add( attr( c_attr_entry ) );
  r->merge( Utils::magic_ptr<Entry>( const_cast< Entry& >( *this ) ), inherit );
  for( t_attrs::iterator i( r->m_attrs.begin() ); i!=r->m_attrs.end(); ++i ) {
    (*i).second = (*i).second->clone();
  }
  return r;
}

Entry::subdataset_info_t Entry::subdataset( Path const & p ) const {
  subdataset_info_t sdsets;
  if( !exists( c_attr_subdataset ) ) {
    return sdsets;
  } else {
    Token::List const & sdset( attr( c_attr_subdataset )->value()->toList() );
    for( int i(0); i!=sdset.length(); ++i ) {
      std::string const & v( sdset.get(i).toString().value() );
      if( v=="." ) {
	sdsets.push_back( SubdatasetInfo( Path( p.asString() + attr( c_attr_entry )->valuestr() ), v ) );
      } else {
	Path p2 = p + v;
	sdsets.push_back( SubdatasetInfo( p2, v ) );
      }
    }
  }
  return sdsets;
}
