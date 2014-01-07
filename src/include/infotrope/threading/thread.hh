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
#ifndef INFOTROPE_THREADING_THREAD_HH
#define INFOTROPE_THREADING_THREAD_HH

#include <pthread.h>

namespace Infotrope {
  namespace Threading {
    
    class Thread {
    public:
      Thread() : m_running( false ) {}
      
      
      virtual ~Thread() {
	if ( m_running ) stop();
      }
      
    private:
      Thread( Thread & ) {}
      
      
      
    public:
      void start();
      void * stop();
      
      void run();
      virtual void * runtime() = 0;
      
      static void * launcher( void * );
      
      bool operator==( Thread const & );
      bool isme() const;
      void kill( int sig ) const;
    private:
      pthread_t m_thread;
      bool m_running;
    };
  }
}

#endif
