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
#ifndef INFOTROPE_DATASTORE_PATH_HH
#define INFOTROPE_DATASTORE_PATH_HH

#include <string>
#include <list>
#include <ostream>

#include <infotrope/utils/stringrep.hh>

namespace Infotrope {
  
  namespace Data {
    
    class Path {
    public:
      typedef enum {
	ByOwner,
	ByClass
      } t_view;
      Path( std::string const & );
      Path( const char * const );
      Path( Utils::StringRep::entry_type const & );
      Path( std::list < Utils::StringRep::entry_type > const & );
      Path( Utils::StringRep::entry_type const &, Utils::StringRep::entry_type const & );
      Path( std::list < Utils::StringRep::entry_type > const &, Utils::StringRep::entry_type const & );
      
      std::string const & asString() const {
	return m_string.get();
      }
      Utils::StringRep::entry_type const & asStringRep() const {
	return m_string;
      }
      std::list < Utils::StringRep::entry_type > const & asList() const {
	return m_list;
      }
      
      Path parent() const;
      Utils::StringRep::entry_type const & mybit() const {
	if ( m_list.empty() ) {
	  throw std::string( "My bit requested from top level path." );
	}
	return m_mybit;
      }
      
      Path canonicalize() const {
	validate();
	if( m_translatable && ( m_view==ByClass ) ) {
	  return toByowner();
	}
	return *this;
      }
      Path translate( t_view v ) const {
	if ( v == ByOwner ) {
	  return toByowner();
	} else {
	  return toByclass();
	}
      }
      bool isView( t_view v ) const {
	return m_view == v;
      }
      t_view view() const {
	return m_view;
      }
      
      bool translatable( t_view const & ) const {
	return m_translatable;
      }
      
      bool has_user() const {
	return m_have_user;
      }
      bool has_owner() const {
	return m_have_owner;
      }
      Utils::StringRep::entry_type const & owner() const {
	if ( !has_owner() ) {
	  throw std::string( "Owner requested from path with no owner." );
	}
	return m_owner;
      }
      void validate() const;
      Path replace_user( std::string const & ) const;
      
      static Path const & get_path( std::string const & );
      static Path const & get_path( std::string const &, std::string const & );
      void write( std::ostream & o ) const {
	o << m_string;
      }
      
      bool operator<( Path const & p ) const {
	return m_string.ptr()<p.m_string.ptr();
      }
      
      bool operator==( Path const & p ) const {
	return p.m_string == m_string;
      }
      bool operator==( std::string const & s ) const {
	return m_string == s;
      }
      bool operator==( const char * s ) const {
	return m_string == s;
      }
      bool operator==( std::list < Utils::StringRep::entry_type > const & l ) const {
	return !( *this != l );
      }
      
      bool operator!=( Path const & p ) const {
	return p.m_string != m_string;
      }
      bool operator!=( std::string const & s ) const {
	return m_string != s;
      }
      bool operator!=( const char * s ) const {
	return m_string != s;
      }
      bool operator!=( std::list < Utils::StringRep::entry_type > const & l ) const {
	if ( l.empty() != m_list.empty() ) {
	  return true;
	}
	//if( l.size()!=size() ) {
	//  return true;
	//}
	
	std::list < Utils::StringRep::entry_type > ::const_iterator it( l.begin() );
	std::list < Utils::StringRep::entry_type > ::const_iterator me( m_list.begin() );
	
	for ( ; ; ++it, ++me ) {
	  if ( it == l.end() ) {
	    return me != m_list.end();
	  }
	  if ( me == m_list.end() ) {
	    return true;
	  }
	  if ( !( ( *it ) == ( *me ) ) ) {
	    return true;
	  }
	}
      }
      
      typedef std::list < Utils::StringRep::entry_type > ::const_iterator const_iterator;
      const_iterator begin() const {
	return m_list.begin();
      }
      const_iterator end() const {
	return m_list.end();
      }
      unsigned long int size() const {
	if ( !m_length ) {
	  m_length = m_list.size();
	}
	return m_length;
      }
      
      bool isRemote() const {
	return !m_netloc.empty();
      }
      Path operator+( Utils::StringRep::entry_type const & e ) const {
	std::list<Utils::StringRep::entry_type> tmp( m_list );
	tmp.push_back( e );
	return Path( tmp );
      }
      
      Path operator+( std::string const & ) const;
    protected:
      void analyze();
      void decompose();
      void compose();
      Path toByowner() const;
      Path toByclass() const;
      
    private:
      Utils::StringRep::entry_type m_string;
      std::list < Utils::StringRep::entry_type > m_list;
      Utils::StringRep::entry_type m_mybit;
      bool m_have_user;
      Utils::StringRep::entry_type m_user;
      bool m_have_owner;
      Utils::StringRep::entry_type m_owner;
      mutable unsigned long int m_length;
      t_view m_view;
      bool m_translatable;
      std::string m_netloc;
    };
    
    inline std::ostream & operator<<( std::ostream & o, Path const & p ) {
      p.write( o );
      return o;
    }
    
  }
  
}

#endif
