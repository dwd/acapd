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
#include <infotrope/threading/thread.hh>
#include <string>
#include <stdexcept>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>

using namespace Infotrope::Threading;

void Thread::start() {
  if ( !pthread_create( &m_thread, NULL, ::Infotrope::Threading::Thread::launcher, reinterpret_cast < void * > ( this ) ) ) {
    m_running = true;
  } else {
    throw std::runtime_error( std::string( "Pthread create failed: " ) + std::strerror( errno ) );
  }
}

void * Thread::stop() {
  void * ret;
  if( pthread_join( m_thread, &ret ) ) {
    throw std::runtime_error( std::string( "Pthread join failed: "  ) + std::strerror( errno ) );
  }
  return ret;
}

void * Thread::launcher( void * o ) {
  Thread * me( reinterpret_cast < ::Infotrope::Threading::Thread * > ( o ) );
  me->run();
  return(0);
}

void Thread::run() {
  try {
    runtime();
  } catch ( ... ) {
  }
  m_running = false;
}

bool Thread::isme() const {
  return pthread_equal( m_thread, pthread_self() );
}

void Thread::kill( int sig ) const {
  if( m_running ) {
    pthread_kill( m_thread, sig );
  }
}
