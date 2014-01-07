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

#ifndef INFOTROPE_UTILS_OSTREAM_PTR_HH
#define INFOTROPE_UTILS_OSTREAM_PTR_HH
#include <sstream>
#include <iostream>

namespace Infotrope {
  namespace Utils {
    
    class ostream_ptr {
    private:
      std::ostringstream m_real;
    public:
      ostream_ptr() : m_real() {
      }
      
      template<typename Tp> ostream_ptr & operator<<( Tp const & p ) {
	m_real << p;
	return *this;
      }
      
      ~ostream_ptr() {
	std::cerr << m_real << std::endl;
      }
    };
  }
}
#endif
