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
#ifndef INFOTROPE_SERVER_RETURN_HH
#define INFOTROPE_SERVER_RETURN_HH

#include <infotrope/utils/stringrep.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <list>
#include <utility>
#include <infotrope/server/token.hh>

namespace Infotrope {
  namespace Data {
    class Entry;
    class Attribute;
    class Dataset;
  }
  namespace Server {
    
    class Return {
    public:
      typedef std::list< std::pair< Utils::StringRep::entry_type, std::list< std::string > > > t_return ;
    private:
      t_return m_return ;
      Utils::StringRep::entry_type m_login;
      
    public:
      Return( Token::Token const &, Utils::StringRep::entry_type const & login );
      
      void parse( Token::Token const & );
      
      Utils::magic_ptr<Token::Token> fetch_attr_metadata( Data::Dataset const &, std::string const &, Utils::magic_ptr<Data::Attribute> const &, Utils::StringRep::entry_type const & ) const;
      Utils::magic_ptr<Token::Token> fetch_attr_metadata( Data::Dataset const &, std::list<std::string> const &, Utils::magic_ptr<Data::Attribute> const &, Utils::StringRep::entry_type const & ) const;
      Utils::magic_ptr<Token::Token> fetch_entry_metadata( Data::Dataset const &, Utils::magic_ptr<Data::Entry> const &, std::pair< Utils::StringRep::entry_type, std::list<std::string> > const & ) const;
      Utils::magic_ptr<Token::Token> fetch_entry_metadata( Data::Dataset const &, Utils::magic_ptr<Data::Entry> const & ) const;
      
    };
  }
}

#endif
