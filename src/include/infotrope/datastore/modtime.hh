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
#ifndef INFOTROPE_DATASTORE_MODTIME_HH
#define INFOTROPE_DATASTORE_MODTIME_HH

#include <string>
#include <ctime>

namespace Infotrope {
  namespace Data {
    
    class Modtime {
    public:
      Modtime( std::string const & );
      Modtime();
      
      bool operator<( Modtime const & ) const;
      bool operator>( Modtime const & ) const;
      bool operator==( Modtime const & ) const;
      
      std::string const & asString() const {
	if( m_string.empty() ) {
	  stringify();
	}
	return m_string;
      }
      
      void bump();
      
      static Modtime const & modtime();
      
    protected:
      void stringify() const;
      void fromTm( struct std::tm & );
    private:
      mutable std::string m_string;
      unsigned long int m_year;    // 4
      unsigned long int m_month;   // 2
      unsigned long int m_day;     // 2
      unsigned long int m_hour;    // 2
      unsigned long int m_minute;  // 2
      unsigned long int m_second;  // 2
      unsigned long int m_tie;     // 3
    };
  }
}

#endif
