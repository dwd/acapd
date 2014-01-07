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
#ifndef INFOTROPE_THREADING_RW_LOCK_HH
#define INFOTROPE_THREADING_RW_LOCK_HH
#include <map>
#include <pthread.h>

/*
  Read/Write Lock.
  Currently this relies on Linux Threads for certain semantics.
  Eventually I'll make these compile-time detirmined, as best I can.
  The semantics marked with a * are therefore to be considered
  non-portable for now. "You" below means "The current thread".
  
  Semantics are:
  
  - A READ lock is a shared lock, many threads may hold a READ lock on a
  resource at the same time.
  - A WRITE lock is an exclusive lock. Only one thread may hold a WRITE lock
  at any given time, and no READ locks may be in progress in any other threads.
  
  1) All READ locks are recursive. In other words, if you already hold a
  READ lock, you can always get another, pretty well instantly. This seems
  pretty portable.
  2) WRITE locks take priority over "new" READ locks. In practise, this means
  that if there are WRITE locks waiting, and you are trying to get a READ lock,
  you'll block unless:
  2a) You already hold a READ lock.
  2b) You already hold a WRITE lock.
  3) WRITE locks will block until existing READ locks expire.
  4) WRITE locks are recursive, too. READ or WRITE locks can always be got if you 
  already hold a WRITE, etc.
  
*/

#ifdef DWDTH_DEBUG
#include <iostream>
#include <infotrope/threading/lock.hh>
namespace {
  Infotrope::Threading::Mutex & debug_lock() {
    static Infotrope::Threading::Mutex m;
    return m;
  }
}
#endif

namespace Infotrope {
  namespace Threading {
  
    class RWMutex {
    private:
      mutable pthread_mutex_t m_mutex;
      mutable pthread_cond_t m_cond;
      mutable pthread_cond_t m_cond_write;
      typedef std::map < pthread_t, long unsigned int > t_readers;
      mutable t_readers * m_readers;
      unsigned long int m_write_pend;
      unsigned long int m_writer;
      pthread_t m_who;
    public:
      RWMutex() : m_readers( 0 ), m_write_pend( 0 ), m_writer( 0 ) {
	pthread_mutex_init( &m_mutex, NULL );
	pthread_cond_init( &m_cond, NULL );
	pthread_cond_init( &m_cond_write, NULL );
      }
      ~RWMutex() {
	pthread_cond_destroy( &m_cond );
	pthread_cond_destroy( &m_cond_write );
	pthread_mutex_destroy( &m_mutex );
      }
      
      void acquire_write() {
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "** 01 " << pthread_self() << " " << this << " Asked for WRITE lock:\n";
	}
#endif
	pthread_mutex_lock( &m_mutex );
	long unsigned int read_count( 0 );
	++m_write_pend;
	if( m_readers ) {
	  t_readers::iterator i( m_readers->find( pthread_self() ) );
	  if ( i != m_readers->end() ) {
	    read_count = ( *i ).second;
	    m_readers->erase( i );
	  }
	  if( m_readers->empty() ) {
	    delete m_readers;
	    m_readers = 0;
	  }
	}
	for ( ; ; ) {
	  if ( ( m_readers && !m_readers->empty() ) || ( m_writer && (m_who!=pthread_self()) ) ) {
#ifdef DWDTH_DEBUG
	    {
	      Infotrope::Threading::Lock l__inst( debug_lock() );
	      std::cout << "** 02 " << pthread_self() << " " << this << "     Readers still, waiting for lock.\n";
	      std::cout << "** 02.5 " << pthread_self() << " " << this << "    " << (m_readers?m_readers->size():0) << " readers, " << m_writer << " writers, writer " << m_who << std::endl;
	    }
#endif
	    pthread_cond_wait( &m_cond, &m_mutex );
	  } else {
#ifdef DWDTH_DEBUG
	    {
	      Infotrope::Threading::Lock l__inst( debug_lock() );
	      std::cout << "** 03 " << pthread_self() << " " << this << "     No readers, taking lock.\n";
	    }
#endif
	    if ( read_count ) {
	      if( !m_readers ) {
		m_readers = new t_readers();
	      }
	      (*m_readers)[ pthread_self() ] = read_count;
	    }
	    // Now register myself, and unlock.
	    m_writer++;
	    m_who = pthread_self();
	    pthread_mutex_unlock( &m_mutex );
	    break;
	  }
	}
      }
      
      void acquire_read() const {
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "++ 04 " << pthread_self() << " " << this << "Asked for READ lock:\n";
	}
#endif
	pthread_mutex_lock( &m_mutex );
	
	while ( m_write_pend || m_writer ) { // Writers pending have priority.
#ifdef DWDTH_DEBUG
	  {
	    Infotrope::Threading::Lock l__inst( debug_lock() );
	    std::cout << "++ 04.5 " << pthread_self() << " " << this << "   Pending write locks (" << m_write_pend << "), waiting.\n";
	  }
#endif
	  if( m_readers ) {
	    t_readers::const_iterator i( m_readers->find( pthread_self() ) );
	    if ( i != m_readers->end() ) {
#ifdef DWDTH_DEBUG
	      {
		Infotrope::Threading::Lock l__inst( debug_lock() );
		std::cout << "++ 04.6 " << pthread_self() << " " << this << "   Already reading in this thread!\n";
	      }
#endif
	      break;
	    }
	  }
	  if( m_writer && (pthread_self()==m_who) ) {
#ifdef DWDTH_DEBUG
	    {
	      Infotrope::Threading::Lock l__inst( debug_lock() );
	      std::cout << "++ 04.7 " << pthread_self() << " " << this << " Already writing in this thread!\n";
	    }
#endif
	    break;
	  }
	  pthread_cond_wait( &m_cond_write, &m_mutex );
	}
	if( !m_readers ) {
	  m_readers = new t_readers();
	}
	++((*m_readers)[ pthread_self() ]);
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "++ 05 " << pthread_self() << " " << this << " Now " << (m_readers?m_readers->size():0) << " read locks.\n";
	}
#endif
	pthread_mutex_unlock( &m_mutex );
      }
      
      void release_read() const {
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "++ 06 " << pthread_self() << " " << this << " Releasing READ lock:\n";
	}
#endif
	pthread_mutex_lock( &m_mutex );
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "++ 06.5 " << pthread_self() << " " << this << " (Were " << (m_readers?m_readers->size():0) << " locks).\n";
	  std::cout << "++ 06.6 " << pthread_self() << " " << this << " (I have " << (m_readers?((*m_readers)[pthread_self()]):0) << " locks myself.\n";
	}
#endif
	if ( 0 == ( --((*m_readers)[ pthread_self() ]) ) ) {
#ifdef DWDTH_DEBUG
	  {
	    Infotrope::Threading::Lock l__inst( debug_lock() );
	    std::cout << "++ 07 " << pthread_self() << " " << this << " No more read locks for this thread.\n";
	  }
#endif
	  m_readers->erase( m_readers->find( pthread_self() ) );
	  if( m_readers->empty() ) {
	    delete m_readers;
	    m_readers = 0;
	  }
	}
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l__inst( debug_lock() );
	  std::cout << "++ 08 " << pthread_self() << " " << this << " Now " << (m_readers?m_readers->size():0) << " locks left.\n";
	}
#endif
	if ( m_write_pend ) {
#ifdef DWDTH_DEBUG
	  {
	    Infotrope::Threading::Lock l__inst( debug_lock() );
	    std::cout << "++ 08.5 " << pthread_self() << " " << this << " Pending write locks, signalling writers.\n";
	  }
#endif
	  pthread_cond_signal( &m_cond );
	}
	pthread_mutex_unlock( &m_mutex );
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l_inst( debug_lock() );
	  std::cout << "++ 09 " << pthread_self() << " " << this << " Finished reading.\n";
	}
#endif
      }
      
      void release_write() {
	pthread_mutex_lock( &m_mutex );
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l_inst( debug_lock() );
	  std::cout << "** 10 " << pthread_self() << " " << this << " Releasing WRITE lock.\n";
	  std::cout << "** 10.5 " << pthread_self() << " " << this << " Currently holding " << m_writer << " locks.\n";
	}
#endif
	--m_write_pend;
	--m_writer;
	if ( !m_write_pend ) {
	  pthread_cond_broadcast( &m_cond_write );
	} else {
	  pthread_cond_signal( &m_cond );
	}
	pthread_mutex_unlock( &m_mutex );
#ifdef DWDTH_DEBUG
	{
	  Infotrope::Threading::Lock l_inst( debug_lock() );
	  std::cout << "** 11 " << pthread_self() << " " << this << " Finished writing.\n";
	}
#endif
      }
    };
    
    class ReadLock {
    private:
      RWMutex const & m_mutex;
    public:
      ReadLock( RWMutex const & l ) : m_mutex( l ) {
	m_mutex.acquire_read();
      }
      ~ReadLock() {
	m_mutex.release_read();
      }
    };
    
    class WriteLock {
    private:
      RWMutex & m_mutex;
    public:
      WriteLock( RWMutex & l ) : m_mutex( l ) {
	m_mutex.acquire_write();
      }
      ~WriteLock() {
	m_mutex.release_write();
      }
    };
  }
}
#endif
