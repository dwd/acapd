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
#include <infotrope/utils/stringrep.hh>
#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#endif
#include <map>

namespace {
  typedef std::map < std::string, std::string * > t_stringmap;
  t_stringmap & stringmap() {
    static t_stringmap s;
    return s;
  }
#ifdef _REENTRANT
  Infotrope::Threading::Mutex & mutex() {
    static Infotrope::Threading::Mutex m;
    return m;
  }
#endif
}

std::string * Infotrope::Utils::StringRep::find( std::string const & s ) {
#ifdef _REENTRANT
  Infotrope::Threading::Lock l__inst( mutex() );
#endif
  t_stringmap::const_iterator i( stringmap().find( s ) );
  if ( stringmap().end() == i ) {
    std::string * ns( new std::string( s ) );
    stringmap()[ s ] = ns;
    return ns;
  }
  return ( *i ).second;
}
