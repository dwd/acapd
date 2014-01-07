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
#ifndef INFOTROPE_DATASTORE_ENTRY_HH
#define INFOTROPE_DATASTORE_ENTRY_HH

#include <infotrope/datastore/attribute.hh>
#include <infotrope/datastore/path.hh>
#include <map>

namespace Infotrope {
  
  namespace Data {
    
    class Entry {
    public:
      typedef Utils::magic_ptr < Attribute > t_attr;
      typedef Attribute::t_name t_attr_name;
      Entry();
      Entry( Utils::StringRep::entry_type const & name );
      
      t_attr const & operator[]( t_attr_name const & name ) const {
	return attr( name );
      }
      t_attr const & attr( t_attr_name const & name ) const;
      bool exists_pure( t_attr_name const & name ) const {
	return m_attrs.find( name ) != end();
      }
      bool exists( t_attr_name const & name ) const {
	const_iterator i( m_attrs.find( name ) );
	if ( i == m_attrs.end() ) {
	  return false;
	}
	if ( ( *i ).second.ptr() ) {
	  if ( ( *i ).second->value()->isNil() && ( *i ).second->acl()->isNil() ) {
	    return false;
	  }
	  return true;
	}
	return false;
      }
      static t_attr s_noattr; // NULL pointer.
      t_attr const & attr2( t_attr_name const & name ) const {
	const_iterator i( m_attrs.find( name ) );
	if( i == m_attrs.end() ) {
	  return s_noattr;
	}
	if( (*i).second.ptr() ) {
	  if( (*i).second->value()->isNil() && (*i).second->acl()->isNil() ) {
	    return s_noattr;
	  }
	  return (*i).second;
	}
	return s_noattr;
      }
      
      struct SubdatasetInfo {
	SubdatasetInfo( Path const & p, std::string const & s )
	  : path( p ), original(s) {
	}
	Path path;
	std::string const & original;
      };
      typedef std::list< SubdatasetInfo > subdataset_info_t;
      subdataset_info_t subdataset( Path const & ) const;
      
      void add( t_attr const & );
      void add( Attribute * a ) {
	add( Utils::magic_ptr < Attribute > ( a ) );
      }
      // For STORE DEFAULT
      void store_default( t_attr_name const & name );
      void store_acl_default( t_attr_name const & name );
      bool attr_isdefault( t_attr_name const & ) const;
      bool acl_isdefault( t_attr_name const & ) const;
      // For STORE NIL
      void store_nil( t_attr_name const & name );
      void store_acl_nil( t_attr_name const & name );
      bool attr_isnil( t_attr_name const & ) const;
      bool acl_isnil( t_attr_name const & ) const;
      // Remove it entirely.
      void erase( t_attr_name const & name );
      
      void merge( Utils::magic_ptr < Entry > const &, bool );
      Utils::magic_ptr < Entry > clone( bool ) const;
      
      typedef std::map < t_attr_name, t_attr, Utils::StringRep::entry_type_less > t_attrs;
      
      //typedef t_attrs::iterator iterator;
      typedef t_attrs::const_iterator const_iterator;
      const_iterator begin() const {
	return m_attrs.begin();
      }
      const_iterator end() const {
	return m_attrs.end();
      }
    private:
      t_attrs m_attrs;
    };
  }
}

#endif
