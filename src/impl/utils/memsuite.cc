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

#include <infotrope/utils/memsuite.hh>
#include <cstring>
#include <map>
#ifdef WITH_ALLOCATOR
#include <memory>
#endif

#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#endif

using namespace Infotrope;

#ifdef _REENTRANT

void * Memsuite::mutex_alloc() {
  return new Threading::Mutex;
}

int Memsuite::mutex_lock( void * m ) {
  reinterpret_cast<Threading::Mutex *>( m )->acquire();
  return 0;
}

int Memsuite::mutex_release( void * m ) {
  reinterpret_cast<Threading::Mutex *>( m )->release();
  return 0;
}

void Memsuite::mutex_free( void * m ) {
  delete reinterpret_cast<Threading::Mutex *>( m );
}

namespace {
  Threading::Mutex & s_mutex() {
    static Threading::Mutex s;
    return s;
  }
}

#define GETLOCK Threading::Lock kiuysahv__lkhsadgvuas( s_mutex() )

#else

#define GETLOCK (0)
// Just to stop compilation errors on some compilers. I think.

#endif

typedef std::map<void *,unsigned long> allocmap_t;

allocmap_t & g_allocmap() {
  static allocmap_t s;
  return s;
}

void * Memsuite::malloc( unsigned long l ) {
  GETLOCK;
  char * p( new char[l] );
  g_allocmap()[ p ] = l;
  return p;
}

void * Memsuite::calloc( unsigned long l, unsigned long c ) {
  return std::memset( Memsuite::malloc( l*c ), 0, l*c );
}

void Memsuite::free( void * p1 ) {
  GETLOCK;
  allocmap_t::iterator i( g_allocmap().find(p1) );
  if( i==g_allocmap().end() ) {
    return;
  }
  delete[] reinterpret_cast<char *>( (*i).first );
  g_allocmap().erase( i );
}

void * Memsuite::realloc( void * p, unsigned long l ) {
  if( l==0 ) {
    Memsuite::free( p );
    return 0;
  }
  void * q( Memsuite::malloc( l ) );
  if( p && q ) {
    allocmap_t::iterator i( g_allocmap().find(p) );
    if( i==g_allocmap().end() ) {
      Memsuite::free( q );
      return 0;
    }
    unsigned long size( (*i).second );
    if( l<(*i).second ) {
      size = l;
    }
    std::memcpy( q, p, size );
    Memsuite::free( p );
  }
  return q;
}

void * Memsuite::malloc( unsigned int x ) {
  return Memsuite::malloc(static_cast<long unsigned int>(x));
}

void * Memsuite::realloc( void * y, unsigned int x ) {
  return Memsuite::realloc( y, static_cast<long unsigned int>(x) );
}
