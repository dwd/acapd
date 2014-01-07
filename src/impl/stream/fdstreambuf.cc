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
#define FDSTREAMBUF_IMPL
#include <infotrope/stream/fdstreambuf.hh>
#ifdef INFOTROPE_QC
#include <infotrope/stream/fdstreambuf.tcc>
#endif
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include <iostream>

namespace Infotrope {
  namespace Stream {
    namespace Exception {
      fderror::fderror( int e, int fd, std::string const & s )
	: runtime_error( s ), m_errno( e ), m_fd( fd ) {}
      
      
      
      const char * fderror::what() const throw() {
	std::ostringstream ss;
	ss << runtime_error::what() << std::endl << "\t" << m_errno << " :: " << strerror( m_errno ) << "\n\tOn file descriptor: " << m_fd;
	char * t( new char[ ss.str().length() + 1 ] );
	std::memcpy( t, ss.str().c_str(), ss.str().length() + 1 );
	return t;
      }
    }
    
#ifdef INFOTROPE_QC
    template class basic_fdstreambuf<char>;
#endif
  }
}
