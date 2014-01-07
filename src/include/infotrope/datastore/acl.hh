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
#ifndef INFOTROPE_DATASTORE_ACL_HH
#define INFOTROPE_DATASTORE_ACL_HH

#include <string>
#include <map>

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/utils/stringrep.hh>
#include <infotrope/server/token.hh>

namespace Infotrope {
  namespace Data {
    class Right {
    protected:
      Right( std::string const & name, unsigned long int mask, char ch );
    public:
      ~Right();
      typedef enum {
	search = 'x',
	read = 'r',
	write = 'w',
	insert = 'i',
	administer = 'a',
	hint_literal = '0'
      } right_name;
      
      unsigned long int mask() const {
	return m_mask;
      }
      char ch() const {
	return m_ch;
      }
      std::string const & name() const {
	return m_name;
      }
      
      static Right const & right( right_name );
      static Right const & right( char );
      // friend static Right const & Right::right( char );
      
    private:
      std::string m_name;
      unsigned long int m_mask;
      char m_ch;
    };
    
    class RightList {
    public:
      RightList();
      RightList( std::string const & );
      RightList const & operator|=( RightList const & r ) {
	m_rights |= r.m_rights;
	return ( *this );
      }
      RightList const & operator+=( RightList const & r ) {
	return ( *this ) |= r;
      }
      RightList const & operator-=( RightList const & r ) {
	m_rights ^= ( m_rights & r.m_rights );
	return *this;
      }
      std::string asString() const;
      bool have_right( Right const & ) const;
      bool have_right( char ) const;
      bool have_rights( RightList const & ) const;
      bool have_rights( std::string const & s ) const {
	return have_rights( RightList( s ) );
      }
      bool operator == ( RightList const & rhs ) const {
	return m_rights == rhs.m_rights;
      }
      
    private:
      unsigned long int m_rights;
    };
    
    class Acl {
    public:
      Acl();
      Acl( Utils::magic_ptr<Token::Token> const & );
      
      Utils::magic_ptr<Token::Token> tokens() const;
      void tokens( Utils::magic_ptr<Token::Token> const & );
      
      RightList myrights( Utils::StringRep::entry_type const & user, Utils::StringRep::entry_type const & owner );
      
      Utils::magic_ptr<Token::Token> const & name() const {
	return m_name;
      }
      bool isNil() const {
	return !this || m_isnil;
      }
      void name( std::string const & dataset );
      void name( std::string const & dataset,
		 std::string const & attribute );
      void name( std::string const & dataset,
		 std::string const & attribute,
		 std::string const & entry );
      
      bool have_authid( Utils::StringRep::entry_type const & );
      void remove( Utils::StringRep::entry_type const & );
      void grant( Utils::StringRep::entry_type const &, RightList const & );
      
      typedef std::map<Utils::StringRep::entry_type,RightList,Utils::StringRep::entry_type_less> t_map;
      typedef t_map::const_iterator const_iterator;
      
      const_iterator begin() const {
	return m_map.begin();
      }
      const_iterator end() const {
	return m_map.end();
      }
      bool operator == ( Acl const & lhs ) const {
	return m_map == lhs.m_map;
      }
      
    private:
      RightList myrights_calc( Utils::StringRep::entry_type const &, Utils::StringRep::entry_type const & );
      t_map m_map;
      Utils::magic_ptr<Token::Token> m_name;
      bool m_isnil;
      std::map<Utils::StringRep::entry_type,t_map,Utils::StringRep::entry_type_less> m_rights_cache;
    };
    
  }
}

#endif


