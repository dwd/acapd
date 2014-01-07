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
#ifndef INFOTROPE_UTILS_CAST_EXCEPTION_HH
#define INFOTROPE_UTILS_CAST_EXCEPTION_HH

#include <typeinfo>
#include <string>

namespace Infotrope {
  namespace Utils {
    
    namespace Exceptions {
      
      class cast : public std::bad_cast {
      public:
	cast( std::string const & really,
	      std::string const & attempted );
	cast( std::string const & really,
	      std::string const & attempted,
	      std::string const & lookslike );
	virtual ~cast() throw() {}
	
	
	
	const char * what() const throw();
      private:
	std::string const m_really;
	std::string const m_attempted;
	std::string const m_lookslike;
      };
      
    }
    
  }
}

#endif
