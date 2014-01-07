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
 *  Copyright  2003-2004 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/
#ifndef INFOTROPE_UTILS_MAGIC_PTR_HH
#define INFOTROPE_UTILS_MAGIC_PTR_HH

#include <memory>
#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#endif
#include <limits>
#include <stdexcept>

#include <infotrope/utils/cast-exception.hh>

#ifdef _REENTRANT
#define INFOTROPE_MP_LOCK(x) Infotrope::Threading::Lock l__inst( (x)->lock() )
#else
#define INFOTROPE_MP_LOCK(x) (x) /* Try to ensure same side-effects... */
#endif

namespace Infotrope {
  namespace Utils {
    template< typename T > class magic_ptr_holder {
    public:
#ifdef _REENTRANT
      typedef Infotrope::Threading::Mutex t_lock;
#endif
      typedef unsigned long int t_counter;
      typedef T t_type;
      typedef magic_ptr_holder<int> t_forwarder;
      
      magic_ptr_holder() throw()
	: m_ocount( std::numeric_limits<t_counter>::max() ),
	  m_scount(0),
	  m_obj(0),
	  m_forwarder(0)
#ifdef _REENTRANT
,
	  m_lock()
#endif
      {}
      
      magic_ptr_holder( t_type * t ) throw()
	: m_ocount( std::numeric_limits<t_counter>::max() ),
	  m_scount(0),
	  m_obj(0),
	  m_forwarder(0)
#ifdef _REENTRANT
,
	  m_lock()
#endif
      {
	m_obj = t;
	m_ocount = 0;
	incr();
      }
      
      magic_ptr_holder( t_type & t ) throw()
	: m_ocount( std::numeric_limits<t_counter>::max() ),
	  m_scount(0),
	  m_obj(0),
	  m_forwarder(0)
#ifdef _REENTRANT
,
	  m_lock()
#endif
      {
	m_obj = &t;
	incr();
      }
      
      magic_ptr_holder( t_type * t, t_forwarder * f ) throw()
	: m_ocount( std::numeric_limits<t_counter>::max() ),
	  m_scount(0),
	  m_obj(0),
	  m_forwarder(0)
#ifdef _REENTRANT
	,
	  m_lock()
#endif
      {
	m_obj = t;
	m_forwarder = f;
	// incr(); // Needs to be done by magic_cast.
      }
      
      void incr() throw() {
	if( m_forwarder ) {
	  INFOTROPE_MP_LOCK( m_forwarder );
	  m_forwarder->incr();
	}
	if( m_ocount!=std::numeric_limits<t_counter>::max() ) {
	  ++m_ocount;
	}
	++m_scount;
      }
      
      bool decr() throw() {
	if( m_forwarder ) {
	  INFOTROPE_MP_LOCK( m_forwarder );
	  if( m_forwarder->m_ocount!=std::numeric_limits<t_counter>::max() ) {
	    --( m_forwarder->m_ocount );
	    if( m_forwarder->m_ocount==0 ) {
	      delete m_obj;
	      m_obj = 0;
	      m_forwarder->m_obj = 0;
	      m_forwarder->m_ocount=std::numeric_limits<t_counter>::max();
	    }
	  }
	  --( m_forwarder->m_scount );
	}
	if( m_ocount!=std::numeric_limits<t_counter>::max() ) {
	  --m_ocount;
	  if( m_ocount==0 ) {
	    delete m_obj;
	    m_obj = 0;
	    m_ocount=std::numeric_limits<t_counter>::max();
	  }
	}
	return (--m_scount)==0;
      }
      
      void incr_weak() throw() {
	if( m_forwarder ) {
	  INFOTROPE_MP_LOCK( m_forwarder );
	  m_forwarder->incr_weak();
	}
	++m_scount;
      }
      
      bool decr_weak() throw() {
	if( m_forwarder ) {
	  INFOTROPE_MP_LOCK( m_forwarder );
	  m_forwarder->decr_weak();
	}
	return (--m_scount)==0;
      }
      
      bool weak() throw() {
	return ptr();
      }
      
      void harden( T * p ) {
	if( m_forwarder ) {
	  throw std::runtime_error( "Attempt to harden weak ref to forwarded holder." );
	}
	m_ocount = 1;
	m_obj = p;
      }
      
      t_type * ptr() throw() {
	return m_obj;
      }
      
#ifdef _REENTRANT
      t_lock & lock() throw() {
	return m_lock;
      }
#endif
      
      // private:
      // Look, these really ought to be private, but there's no way of saying "all template instantiations are friends".
      // So we'll pretend they're private.
      // If you access these directly, I'll beat you up with a large stick.
      t_counter m_ocount;
      t_counter m_scount;
      t_type * m_obj;
      t_forwarder * m_forwarder;
#ifdef _REENTRANT
      t_lock m_lock;
#endif
    };
    
    namespace impl {
      typedef std::allocator< magic_ptr_holder<char> > t_uni_alloc;
      
      static t_uni_alloc * uni_alloc() {
	static t_uni_alloc s_uni_alloc;
	return &s_uni_alloc;
      }
      template< typename T > T * allocate() {
	return reinterpret_cast< T * >( uni_alloc()->allocate( 1 ) );
      }
      template< typename T > void deallocate( T * p ) {
	uni_alloc()->deallocate( reinterpret_cast< magic_ptr_holder<char> * >( p ), 1 );
      }
    }
    
    template< typename T > class magic_ptr {
    public:
      typedef T t_type;
      typedef magic_ptr_holder<t_type> t_holder;
      
      magic_ptr() throw() : m_holder(0) {
      }
      magic_ptr( magic_ptr<t_type> const & p ) throw() : m_holder(0) {
	if( p.m_holder ) {
	  INFOTROPE_MP_LOCK( p.m_holder );
	  m_holder = p.m_holder;
	  if( m_holder ) {
	    m_holder->incr();
	  }
	}
      }
      explicit magic_ptr( t_type * p ) : m_holder(0) {
	try {
	  m_holder = impl::allocate<t_holder>();
	  new(m_holder) t_holder( p );
	} catch(...) {
	  delete p;
	  throw;
	}
      }
      explicit magic_ptr( t_type & p ) : m_holder(0) {
	m_holder = impl::allocate<t_holder>();
	new(m_holder) t_holder( p );
      }
      explicit magic_ptr( t_holder * h ) throw() : m_holder(h) {
	m_holder->incr();
      }
      ~magic_ptr() throw() {
	zap();
      }
      
      void zap() throw() {
	if( m_holder ) {
	  t_holder * q;
	  {
	    INFOTROPE_MP_LOCK( m_holder );
	    q = m_holder;
	    m_holder = 0;
	  }
	  if( q->decr() ) {
	    q->~t_holder();
	    impl::deallocate( q );
	  }
	}
      }
      
      void set( t_type * p ) {
	if( p==ptr() ) return;
	zap();
	try {
	  m_holder = impl::allocate<t_holder>();
	  new( m_holder ) t_holder( p );
	} catch(...) {
	  delete p;
	  throw;
	}
      }
      void set( t_type & p ) {
	if( &p == ptr() ) return;
	zap();
	m_holder = impl::allocate<t_holder>();
	new( m_holder ) t_holder( p );
      }
      void set( magic_ptr<t_type> const & p ) throw() {
	if( p.ptr() == ptr() ) return;
	zap();
	if( p.m_holder ) {
	  INFOTROPE_MP_LOCK( p.m_holder );
	  m_holder = p.m_holder;
	  if( m_holder ) {
	    m_holder->incr();
	  }
	}
      }
      
      t_type * ptr() const throw() {
	if( m_holder ) return m_holder->ptr();
	return 0;
      }
      t_type & get() const throw() {
	return *ptr();
      }
      
      t_type * operator->() const throw() {
	return ptr();
      }
      t_type & operator*() const throw() {
	return get();
      }
      magic_ptr<t_type> & operator=( magic_ptr<t_type> const & p ) throw() {
	set( p );
	return *this;
      }
      magic_ptr<t_type> & operator=( t_type & p ) throw() {
	set( p );
	return *this;
      }
      magic_ptr<t_type> & operator=( t_type * p ) throw() {
	set( p );
	return *this;
      }
      
      operator bool() const throw() {
	return ptr();
      }
      
      t_holder * holder_private() const throw() {
	return m_holder;
      }
      
      bool operator<( magic_ptr<t_type> const & p ) const throw() {
	return m_holder < p.holder_private();
      }
      
    private:
      t_holder * m_holder;
    };
    
    template< typename T, typename Q > magic_ptr<T> magic_cast( magic_ptr<Q> const & q ) {
      if( q ) {
	T * t( dynamic_cast<T*>( q.ptr() ) );
	if( !t ) {
	  throw std::runtime_error( "Whoops. Arsed up." );
	}
	if( reinterpret_cast<unsigned long int>( t )!=reinterpret_cast<unsigned long int>( q.ptr() ) ) {
	  q.holder_private()->incr();
	  magic_ptr_holder<T> * x( impl::allocate< magic_ptr_holder<T> >() );
	  new( x ) magic_ptr_holder<T>( t, reinterpret_cast< magic_ptr_holder<int> * >( q.holder_private() ) );
	  return magic_ptr<T>( x );
	}
      }
      return magic_ptr<T>( reinterpret_cast< typename magic_ptr<T>::t_holder * >( q.holder_private() ) );
    }
    
    template<typename T > class weak_ptr : public std::allocator< magic_ptr_holder<T> > {
    public:
      typedef T t_type;
      typedef magic_ptr_holder<t_type> t_holder;
      
    private:
      // Not actually implemented, just here to scare off misuse.
      weak_ptr( T * );
      weak_ptr( T const & );
    public:
      
      weak_ptr( magic_ptr<T> const & p ) throw()
	: m_holder(0) {
	INFOTROPE_MP_LOCK( p.holder_private() );
	m_holder = p.holder_private();
	m_holder->incr_weak();
      }
      weak_ptr( weak_ptr<T> const & p ) throw()
	: m_holder( 0 ) {
	INFOTROPE_MP_LOCK( p.m_holder );
	m_holder = p.m_holder;
	m_holder->incr_weak();
      }
      ~weak_ptr() throw() {
	zap();
      }
      
      magic_ptr<t_type> harden() const throw() {
	return magic_ptr<t_type>( m_holder );
      }
      template<typename F> bool reacquire( F & f ) throw() {
	INFOTROPE_MP_LOCK( m_holder );
	if( m_holder->weak() ) m_holder->harden( f() );
	return (*this);
      }
      // Strict weak ordering purposes only...
      bool operator<( weak_ptr<T> const & s ) const throw() {
	return m_holder < s.m_holder;
      }
      
    private:
      void zap() throw() {
	if( m_holder ) {
	  INFOTROPE_MP_LOCK( m_holder );
	  t_holder * p( m_holder );
	  m_holder = 0;
	  if( p->decr_weak() ) {
	    p->~t_holder();
	    impl::deallocate( p );
	  }
	}
      }
      
      t_holder * m_holder;
    };
	
#undef INFOTROPE_MP_LOCK

  }
  
}

#endif
