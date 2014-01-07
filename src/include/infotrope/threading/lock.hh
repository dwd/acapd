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
#ifndef INFOTROPE_THREADING_LOCK_HH
#define INFOTROPE_THREADING_LOCK_HH

#ifdef _REENTRANT

#include <pthread.h>

namespace Infotrope {
  namespace Threading {
    
    class Mutex {
    protected:
      mutable pthread_mutex_t m_mutex;
      
    public:
      Mutex() {
	pthread_mutex_init( &m_mutex, NULL );
      }
      
      void acquire() const {
	pthread_mutex_lock( &m_mutex );
      }
      
      void release() const {
	pthread_mutex_unlock( &m_mutex );
      }
      
    public:
      virtual ~Mutex() {
	pthread_mutex_destroy( &m_mutex );
      }
    };
    
    class Lock {
    public:
      Lock( Mutex const & m )
	: m_mutex( m ) {
	  m_mutex.acquire();
	}
      
      ~Lock() {
	m_mutex.release();
      }
      
    private:
      Mutex const & m_mutex;
    };
  }
}

#endif

#endif
