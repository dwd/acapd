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
#ifndef INFOTROPE_THREADING_SIGLOCK_HH
#define INFOTROPE_THREADING_SIGLOCK_HH

#include <infotrope/threading/lock.hh>

namespace Infotrope {
  namespace Threading {
    
    class Mutex;
    
    class SigMutex : public Mutex {
    private:
      pthread_cond_t m_cond;
    public:
      SigMutex() : Mutex() {
	pthread_cond_init( &m_cond, NULL );
      }
      
      void wait( Lock & ) { // Not const, so we can't get a temp.
	pthread_cond_wait( &m_cond, &m_mutex );
      }
      
      void signal( Lock const & ) {
	pthread_cond_signal( &m_cond );
      }
      
      void broadcast( Lock const & ) {
	pthread_cond_broadcast( &m_cond );
      }
      
      ~SigMutex() {
	pthread_cond_destroy( &m_cond );
      }
    };
  }
}

#endif
