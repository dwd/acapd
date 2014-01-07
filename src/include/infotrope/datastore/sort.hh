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
#ifndef ACAP_SORT_HH
#define ACAP_SORT_HH

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/server/token.hh>
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/entry.hh>
#include <list>
#include <utility>

namespace Infotrope {
  namespace Data {
    
    // NIL is usually equally collated with a list, however, when criteria generate a fast sort order,
    // they switch this such that NIL is collated higher than a list.
    // This basically means that criteria can find NIL values quickly.
    
    class Sort {
    public:
      typedef std::list< std::pair< Utils::magic_ptr<Comparators::Comparator>, Utils::StringRep::entry_type > > t_sort_order;
    private:
      void parse( Token::List const & );
      t_sort_order m_sort_order;
      bool m_nil_different;
      
    public:
      Sort();
      Sort( Token::List const &, bool nil_different = false );
      
      bool operator()( std::pair< Utils::magic_ptr<Entry>, Utils::StringRep::entry_type > const & a,
		       std::pair< Utils::magic_ptr<Entry>, Utils::StringRep::entry_type > const & b ) const;
      typedef t_sort_order::const_iterator const_iterator;
      const_iterator begin() {
	return m_sort_order.begin();
      }
      const_iterator end() {
	return m_sort_order.end();
      }
      bool nil_different() const {
	return m_nil_different;
      }
    };
  }
  
}

#endif
