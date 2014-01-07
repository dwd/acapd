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
#include <infotrope/datastore/modtime.hh>
#include <sstream>
#include <ctime>
#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#endif

using namespace Infotrope::Data;
using namespace std;

namespace {
  inline void getdigits( istream & ss, long unsigned int &r, long unsigned int c ) {
    r = 0;
    for ( unsigned int i( 0 ); i < c; ++i ) {
      r *= 10;
      r += ss.get() - '0';
    }
  }
  inline void getdigits( istream & ss, long unsigned int &r ) {
    r = 0;
    for ( ; ; ) {
      int c( ss.get() );
      if ( ss.eof() ) {
	break;
      }
      r *= 10;
      r += c - '0';
    }
  }
  inline std::string putdigits( long unsigned int r, long unsigned int c ) {
    ostringstream ss;
    ss << r;
    string s( ss.str() );
    while ( s.length() < c ) {
      s = "0" + s;
    }
    return s;
  }
}

Modtime::Modtime( string const & s )
  : m_string( s ), m_year( 0 ), m_month( 0 ), m_day( 0 ),
    m_hour( 0 ), m_minute( 0 ), m_second( 0 ), m_tie( 0 ) {
  if ( s.length() < ( 4 + 2 + 2 + 2 + 2 + 2 ) ) {
    throw string( "Modtime value " + s + " is too short." );
  }
  istringstream ss( s );
  getdigits( ss, m_year, 4 );
  getdigits( ss, m_month, 2 );
  getdigits( ss, m_day, 2 );
  getdigits( ss, m_hour, 2 );
  getdigits( ss, m_minute, 2 );
  getdigits( ss, m_second, 2 );
  getdigits( ss, m_tie );
  while ( m_tie > 999 ) {
    m_tie /= 10;
  }
  //stringify();
}

Modtime::Modtime()
  : m_string(), m_year( 0 ), m_month( 0 ), m_day( 0 ),
    m_hour( 0 ), m_minute( 0 ), m_second( 0 ), m_tie( 0 ) {
  struct tm now;
  time_t t;
  time( &t );
  gmtime_r( &t, &now );
  fromTm( now );
  //stringify();
}

void Modtime::stringify() const {
  m_string = putdigits( m_year, 4 );
  m_string += putdigits( m_month, 2 );
  m_string += putdigits( m_day, 2 );
  m_string += putdigits( m_hour, 2 );
  m_string += putdigits( m_minute, 2 );
  m_string += putdigits( m_second, 2 );
  m_string += putdigits( m_tie, 3 );
}

void Modtime::fromTm( struct std::tm & now ) {
  m_year = now.tm_year + 1900;
  m_month = now.tm_mon + 1;   // ACAP modtime is 01-12, tm is 00-11.
  m_day = now.tm_mday;
  m_hour = now.tm_hour;
  m_minute = now.tm_min;
  m_second = now.tm_sec;
}

bool Modtime::operator > ( Modtime const & m ) const {
  if ( m_year < m.m_year ) {
    return false;
  } else if( m_year > m.m_year ) {
    return true;
    
  } else if ( m_month < m.m_month ) {
    return false;
  } else if( m_month > m.m_month ) {
    return true;
    
  } else if ( m_day < m.m_day ) {
    return false;
  } else if( m_day > m.m_day ) {
    return true;
    
  } else if ( m_hour < m.m_hour ) {
    return false;
  } else if( m_hour > m.m_hour ) {
    return true;
    
  } else if ( m_minute < m.m_minute ) {
    return false;
  } else if( m_minute > m.m_minute ) {
    return true;
    
  } else if ( m_second < m.m_second ) {
    return false;
  } else if( m_second > m.m_second ) {
    return true;
    
  } else if ( m_tie <= m.m_tie ) {
    return false;
  }
  return true;
}

bool Modtime::operator < ( Modtime const & m ) const {
  return ( m > (*this) );
}

bool Modtime::operator == ( Modtime const & m ) const {
  if ( m_year == m.m_year ) {
    if ( m_month == m.m_month ) {
      if ( m_day == m.m_day ) {
	if ( m_hour == m.m_hour ) {
	  if ( m_minute == m.m_minute ) {
	    if ( m_second == m_second ) {
	      if ( m_tie == m.m_tie ) {
		return true;
	      }
	    }
	  }
	}
      }
    }
  }
  return false;
}

inline void Modtime::bump() {
  ++m_tie;
  if ( m_tie >= 1000 ) {
    m_tie = 0;
    struct tm now;
    time_t now_time;
    time( &now_time );
    ++now_time;
    gmtime_r( &now_time, &now );
    fromTm( now );
  }
  // stringify();
}

namespace {
  inline Modtime & last_modtime() {
    static Modtime m;
    return m;
  }
#ifdef _REENTRANT
  Infotrope::Threading::Mutex lock;
#endif
}

Modtime const & Modtime::modtime() {
#ifdef _REENTRANT
  Infotrope::Threading::Lock l__inst( lock );
#endif
  Modtime m;
  if( !( m > last_modtime() ) ) {
    m = last_modtime();
    m.bump();
  }
  while( !( m > last_modtime() ) ) {
    m.bump();
  }
  last_modtime() = m;
  return last_modtime();
}
