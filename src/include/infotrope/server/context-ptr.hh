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
#ifndef INFOTROPE_SERVER_CONTEXT_PTR_HH
#define INFOTROPE_SERVER_CONTEXT_PTR_HH

namespace Infotrope {
  namespace Server {
    
    class Worker;
    class Context;
    
    class ContextPtr {
    public:
      ContextPtr( Worker &, std::string const & );
      ContextPtr();
      
      bool valid() const;
      Context & operator*() const;
      Context * operator->() const;
      bool operator<( ContextPtr const & p ) const {
	if ( m_fd < p.m_fd ) {
	  return true;
	} else if ( m_fd == p.m_fd ) {
	  return m_name < p.m_name;
	}
	return false;
      }
      
    private:
      const int m_fd;
      const std::string m_name;
    };
  }
}

#endif
