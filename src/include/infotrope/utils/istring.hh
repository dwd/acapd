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
#ifndef INFOTROPE_UTILS_ISTRING_HH
#define INFOTROPE_UTILS_ISTRING_HH

// This is somewhat adjusted from the version hiding away in the G++ library.
// I've made it use locale information, though.
// This is tricky, since locales aren't actually stored in the string at all.
// Essentially, a string is simply Another Container.
// This doesn't bother me greatly, however, since I just use the global locale,
// which we vaguely assume is going to be sane.
// We could use the classic, or "C", or "POSIX" locales instead with equal
// sanity.
// For my own sanity, I'm using the convenience functions, rather than formally
// extracting the ctype<> facet.

#include <string>
#include <locale>

// Irritatingly, you can, apparently, only output a string through an ostream
// when both share the char_traits...
#include <ostream>

namespace Infotrope {
  namespace Utils {
    
    template < typename _CharT >
    struct ci_char_traits : public std::char_traits < _CharT > {
      typedef typename std::char_traits < _CharT > ::char_type char_type;
      static bool eq( char_type a, char_type b ) {
	std::locale l;
	return std::tolower( a, l ) == std::tolower( b, l );
      }
      static bool ne( char_type a, char_type b ) {
	std::locale l;
	return std::tolower( a, l ) != std::tolower( b, l );
      }
      static bool lt( char_type a, char_type b ) {
	std::locale l;
	return std::tolower( a, l ) < std::tolower( b, l );
      }
      static int compare( const char_type * s1, const char_type * s2, const size_t n ) {
	std::locale l;
	for ( size_t i( 0 ); i != n; ++i ) {
	  char_type a( std::tolower( s1[ i ], l ) );
	  char_type b( std::tolower( s2[ i ], l ) );
	  if ( lt( a, b ) ) {
	    return -1;
	  } else if ( lt( b, a ) ) {
	    return 1;
	  }
	}
	return 0;
      }
      static const char_type * find( const char_type * s, size_t n, const char_type & a ) {
	std::locale l;
	char_type t( std::tolower( a, l ) );
	while ( n-- > 0 && std::tolower( *s, l ) != t ) {
	  ++s;
	}
	return s;
      }
    };
    
    typedef std::basic_string < char, ci_char_traits < char > > istring;
    
    inline std::ostream & operator<<( std::ostream & o, istring const & i ) {
      o << std::string( i.data(), i.size() );
      return o;
    }
    inline bool operator==( std::string const & s, istring const & i ) {
      return ( istring( s.data(), s.size() ) == i );
    }
    inline bool operator==( istring const & i, std::string const & s ) {
      return s == i;
    }
    
    inline istring istring_convert( std::string const & s ) {
      return istring( s.data(), s.size() );
    }
    inline std::string istring_convert( istring const & i ) {
      return std::string( i.data(), i.size() );
    }
  }
}

#endif
