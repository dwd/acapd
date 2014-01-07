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
#include <infotrope/datastore/sort.hh>

using namespace Infotrope::Data;
using namespace Infotrope;
using namespace std;

Sort::Sort()
  : m_sort_order(), m_nil_different( false ) {}

Sort::Sort( Token::List const & l, bool nil_different )
  : m_sort_order(), m_nil_different( nil_different ) {
  parse( l );
}

void Sort::parse( Token::List const & l ) {
  for ( int i( 0 ); i < l.length(); i += 2 ) {
    string comp( l.get( i + 1 ).toString().value() );
    m_sort_order.push_back( make_pair( Comparators::Comparator::comparator( comp ),
				       l.get( i ).toString().value() ) );
  }
}

bool Sort::operator()( pair < Utils::magic_ptr < Entry > , Utils::StringRep::entry_type > const & a,
                       pair < Utils::magic_ptr < Entry > , Utils::StringRep::entry_type > const & b ) const {
  for ( t_sort_order::const_iterator i( m_sort_order.begin() );
	i != m_sort_order.end(); ++i ) {
    using Infotrope::Comparators::Comparator;
    static Utils::magic_ptr<Attribute> s_nil;
    Utils::magic_ptr<Attribute> ta;
    if ( a.first->exists( ( *i ).second ) ) {
      ta = a.first->attr( ( *i ).second );
    } else {
      ta = s_nil;
    }
    Utils::magic_ptr<Attribute> tb;
    if ( b.first->exists( ( *i ).second ) ) {
      tb = b.first->attr( ( *i ).second );
    } else {
      tb = s_nil;
    }
    Comparator::compare_result c( ( *i ).first->order( ta, tb, m_nil_different ) );
    if ( c == Comparator::Lower ) {
      return true;
    }
    if ( c == Comparator::Higher ) {
      return false;
    }
  }
  if( a.second.ptr() < b.second.ptr() ) {   // Defer to something.
    return true;
  }
  return false;
  // This also prevents ties in the case of, for instance, two entries with
  // the same "entry" in results. A formal tie would prevent it being stored
  // in the Subcontext - bad news.
}
