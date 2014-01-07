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
#ifndef INFOTROPE_UTILS_STRINGREP_HH
#define INFOTROPE_UTILS_STRINGREP_HH

#include <infotrope/utils/magic-ptr.hh>
#include <string>

namespace Infotrope {
  namespace Utils {
    
    namespace StringRep {
      
      class entry_type;
      class entry_type_less;
      
      std::string * find( std::string const & );
      
      class entry_type {
      private:
	std::string * m_v;
      public:
	entry_type( const char * s )
	  : m_v( find( s ) ) {}
	
	
	entry_type( std::string const & s )
	  : m_v( find( s ) ) {}
	
	
	entry_type( entry_type const & e )
	  : m_v( e.m_v ) {}
	
	entry_type()
	  : m_v() {}
	
	bool operator<( entry_type const & e ) const {
	  return get() < e.get();
	}
	std::string const & get() const {
	  if( !m_v ) {
	    throw std::string( "Dereference of NULL StringRep::entry_type." );
	  }					
	  return *m_v;
	}
	std::string const * ptr() const {
	  return m_v;
	}
	bool operator==( const char * s ) const {
	  return get() == s;
	}
	bool operator==( std::string const & s ) const {
	  return get() == s;
	}
	bool operator!=( std::string const & s ) const {
	  return get() != s;
	}
	bool operator!=( const char * s ) const {
	  return get() != s;
	}
	bool operator==( Utils::StringRep::entry_type const & e ) const {
	  if ( m_v == e.m_v ) {
	    return true;
	  } else if ( get() == e.get() ) {
	    return true;
	  } else {
	    return false;
	  }
	}
	bool operator!=( Utils::StringRep::entry_type const & e ) const {
	  if ( m_v != e.m_v ) {
	    return true;
	  } else if ( get() != e.get() ) {
	    return true;
	  } else {
	    return false;
	  }
	}
	std::string const * operator->() const {
	  if( !m_v ) {
	    throw std::string( "Dereference of NULL StringRep::entry_type." );
	  }
	  return m_v;
	}
      };
      
      class entry_type_less {
      public:
	bool operator()( entry_type const & a, entry_type const & b ) const {
	  return a.ptr() < b.ptr();
	}
      };
      
      inline std::ostream & operator<<( std::ostream & o, entry_type const & e ) {
	o << e.get();
	return o;
      }
    }
    
  }
}

#endif
