#include <infotrope/utils/file-mapper.hh>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <stdexcept>
#include <exception>

#ifdef TESTING
#include <iostream>
#include <list>
#include <iostream>
#endif

using namespace Infotrope::Utils::MMap;

int const ::Infotrope::Utils::MMap::g_pagesize( getpagesize() );
#ifdef _REENTRANT
pthread_once_t Infotrope::Utils::MMap::ScopeLocality::s_once = PTHREAD_ONCE_INIT;
pthread_key_t Infotrope::Utils::MMap::ScopeLocality::s_current_key;
#else
signed long int ::Infotrope::Utils::MMap::ScopeLocality::s_current(0);
#endif


SimpleAlloc::SimpleAlloc() {
}

SimpleAlloc::SimpleAlloc( std::size_t s, filemapper_ptr<void> p, std::size_t n )
#ifdef _REENTRANT
  : m_size(s), m_finelock()
#else
    : m_size(s)
#endif
{
#ifdef _REENTRANT
    Infotrope::Threading::WriteLock l__inst( m_finelock );
#endif
    if( m_size < sizeof( FreeMarker ) ) {
      throw std::runtime_error( "Cannot allocate sizes below 8 octets." );
    }
    first = filemapper_ptr<FreeMarker>( p );
    if( first ) {
      first->next = 0;
      first->num = n*g_pagesize/s;
    }
    current = 0;
    end = first;
#ifdef SIMPLEALLOC_TRACK
    m_allocs = 0;
#endif
  }

#ifdef _REENTRANT
#ifdef TESTING
#include <infotrope/threading/lock.hh>
namespace {
  Infotrope::Threading::Mutex & debugmutex() {
    static Infotrope::Threading::Mutex m;
    return m;
  }
}
#endif
#endif

filemapper_ptr<void> SimpleAlloc::allocate( std::size_t n, signed long int l, int pass ) {
#ifdef _REENTRANT
  Infotrope::Threading::WriteLock l__inst( m_finelock );
#endif
  if( first && !end ) {
    throw *end;
  }
#ifdef TESTING
#ifdef _REENTRANT
  {
    Infotrope::Threading::Lock l__inst( debugmutex() );
#endif
    std::cerr << "Allocating " << n << " units of " << m_size << " in pass " << pass << ", locality " << l << " " << this << std::endl;
#ifdef _REENTRANT
  }
#endif
#endif
  if( current ) {
    // First pass.
    //#ifdef TESTING_DEBUG
    //std::cerr << "Looking from current.\n";
    //#endif
    filemapper_ptr<FreeMarker> cand( current->next );
    while( cand ) {
      if( cand->num>=n ) {
#ifdef TESTING
	std::cerr << "Found space for " << cand->num << " in candidate.\n";
#endif
	current->next = allocate( cand, n );
	if( cand==end ) {
	  end = current->next;
	  if( !end ) {
	    end = current;
	  }
	}
	if( first && !end ) {
	  throw *end;
	}
	current = 0;
	return filemapper_ptr<void>( cand );
      } else {
	current = cand;
	cand = cand->next;
      }
    }
    if( pass==1 ) {
#ifdef TESTING
      std::cerr << "Pass 1 exhausted, trying to reorder.\n";
#endif
      reorder();
    } else if( pass==2 ) {
#ifdef TESTING
      std::cerr << "Pass 2 exhausted.\n";
#endif
      exhausted( n, l );
      return allocate( n, l, 0 );
    }
  }
  // Okay, failed to find anything.
  // Try looking at first:
#ifdef TESTING_DEBUG
  std::cerr << "Restarting.\n";
#endif
  if( !first ) {
#ifdef TESTING
    std::cerr << "No first, so I'm exhausted.\n";
#endif
    exhausted( n, l );
  }
  if( first->num>=n ) {
    // Okay, allocate here.
#ifdef TESTING
    std::cerr << "Found space for " << first->num << " at first.\n";
#endif
    filemapper_ptr<FreeMarker> p( first );
    first = allocate( first, n );
    if( p==end ) {
      end = first;
    }
    if( first && !end ) {
      throw *end;
    }
    return filemapper_ptr<void>( p );
  }
  current = first;
#ifdef TESTING
  std::cerr << "Ready for next pass.\n";
#endif
  return allocate( n, l, ++pass );
}

void SimpleAlloc::exhausted( std::size_t n, signed long int l ) {
  if( first && !end ) {
    throw *end;
  }
#ifdef TESTING
  std::cerr << "Exhausted for " << n << " units of " << m_size << " in " << l << std::endl;
#endif
  // Should pull from parent, but we're cheating.
  filemapper_ptr<SimpleAlloc> a( g_mmap_man->simplealloc( l, g_pagesize ) );
  std::size_t pages( (m_size*(n+20)+g_pagesize-1)/g_pagesize );
  filemapper_ptr<void> p( a->allocate( pages, l ) );
  if( !p ) {
#ifdef TESTING
    std::cerr << "Exhausted locality!\n";
#endif
    throw std::bad_alloc();
  }
  deallocate( p, pages*g_pagesize/m_size ); // Hack, but works fine.
#ifdef SIMPLEALLOC_TRACK
  m_allocs += pages*g_pagesize/m_size;
#endif
  current = 0;
}

// deallocate marks a chunk as free.
// Note that we only try to merge, or coagulate, blocks which we can easily do so to.
// This makes things much faster, and the data structures much simpler.
// OTOH, it means that for distinctly variable values of 'n', we will tend to fragment
// fairly badly.
void SimpleAlloc::deallocate( filemapper_ptr<void> p, std::size_t n ) {
#ifdef _REENTRANT
  Infotrope::Threading::WriteLock l__inst( m_finelock );
#endif
  if( first && !end ) {
    throw *end;
  }
#ifdef TESTING
  //std::cerr << "MM-- " << p << " //Deallocating " << n << " units of " << m_size << " at " << p << std::endl;
  {
#ifdef _REENTRANT
    Infotrope::Threading::Lock l__inst( debugmutex() );
#endif
    std::cerr << "DS " << p << " " /*<< n << " " << m_size << " "*/ << this << std::endl;
  }
#endif
#ifdef SIMPLEALLOC_TRACK
  m_allocs -= n;
#endif
  filemapper_ptr<FreeMarker> f( p );
  f->next = 0;
  f->num = n;
  // Do we have a quick fix?
  if( !first ) {
#if 0
    {
      Infotrope::Threading::Lock l__inst( debugmutex() );
      std::cerr << "DFE\n";
    }
#endif
    first = f;
    end = f;
  } else if( f->glueme( m_size )==first ) { // Before first.
#if 0
    {
      Infotrope::Threading::Lock l__inst( debugmutex() );
      std::cerr << "DCBF\n";
    }
#endif
    f->num += first->num;
    f->next = first->next;
    if( first==end ) {
      end = f;
    }
    if( first==current ) {
      current = f;
    }
    first = f;
  } else if( first->glueme( m_size )==f ) { // After first.
    first->num += f->num;
#if 0
    {
      Infotrope::Threading::Lock l__inst( debugmutex() );
      std::cerr << "Dealloc coagulating after first.\n";
    }
#endif
  } else if( current && current->glueme( m_size )==f ) { // After current.
    //std::cerr << "Dealloc coagulating after current.\n";
    current->num += f->num;
  } else if( end && end->glueme( m_size )==f ) { // After end.
    //std::cerr << "Dealloc coagulating after end.\n";
    end->num += f->num;
  } else if( end ) {
#if 0
    {
      Infotrope::Threading::Lock l__inst( debugmutex() );
      std::cerr << "Dealloc appending to end.\n";
    }
#endif
    end->next = f;
    end = f;
  } else {
    //std::cerr << "I appear to have accidentally lost your memory.\n";
    int *i(0);
    throw *i;
  }
#if 0
  {
    Infotrope::Threading::Lock l__inst( debugmutex() );
    std::cerr << "Deallocate End " << p << " " << this << std::endl;
  }
#endif
}

// Allocate at this point. Return new FreeMarker.
filemapper_ptr<SimpleAlloc::FreeMarker> SimpleAlloc::allocate( filemapper_ptr<SimpleAlloc::FreeMarker> f, std::size_t n ) {
#ifdef SIMPLEALLOC_TRACK
  m_allocs += n;
#endif
  if( first && !end ) {
    throw *end;
  }
  if( f->num==n ) {
#if 0
    {
      Threading::Lock l__inst( debugmutex() );
      std::cerr << "MM++R " << f << " //Allocating from " << f << " for " << n << " units of " << m_size << " " << this << ".\n";
    }
#endif
    return f->next;
  } else {
#ifdef TESTING_DEBUG
    {
      Threading::Lock l__inst( debugmutex() );
      std::cerr << "MM++P " << f << " //Allocating from " << f << " for " << n << " units of " << m_size << " " << this << ".\n";
    }
#endif
    filemapper_ptr<FreeMarker> p( f->trynext( n*m_size ) );
#ifdef TESTING_DEBUG
    std::cerr << "Got " << p << "\n";
#endif
    p->next = f->next;
    p->num = f->num-n;
#ifdef TESTING_DEBUG
    f->num = n;
    std::cerr << "Glueme is " << f->glueme( m_size ) << "\n";
#endif
    return p;
  }
}

// Coagulate all FreeMarker blocks into largest blocks possible, from both fs and fc.
// Return new FreeMarker list.
void SimpleAlloc::reorder() {
  std::set< filemapper_ptr<FreeMarker> > blocks;
  filemapper_ptr<FreeMarker> fs( first );
  //#ifdef TESTING
  int b(0);
  //std::cerr << "    Inserting free markers from unallocated list.\n";
  //#endif
  for( ;fs;fs=fs->next ) {
    blocks.insert(fs);
    //#ifdef TESTING
    ++b;
    //#endif
  }
  //#ifdef TESTING
  //std::cerr << "    Inserted " << b << " markers.\n";
  //#endif
  for( std::set< filemapper_ptr<FreeMarker> >::const_iterator i( blocks.begin() ); i!=blocks.end(); ++i ) {
    std::set< filemapper_ptr<FreeMarker> >::const_iterator j( i );
    ++j;
    if( j!=blocks.end() ) {
      //#ifdef TESTING_DEBUG
      //std::cerr << "    Next is valid, using it.\n";
      //#endif
      (*i)->next = (*j);
    } else {
      //#ifdef TESTING_DEBUG
      //std::cerr << "    End of list, terminating.\n";
      //#endif
      (*i)->next = 0;
    }
#ifdef TESTING_DEBUG
    std::cerr << "    Next.\n";
#endif
  }
  filemapper_ptr<FreeMarker> start( *blocks.begin() );
  for( filemapper_ptr<FreeMarker> f( start ); f; ) {
    // Coagulation...
    end = f;
    //#ifdef TESTING
    //std::cerr << "    " << f << " for " << f->num << " ending " << f->glueme( m_size ) << " until " << f->next << std::endl;
    if( f->next == f->glueme( m_size ) ) { // If the next one would be where this one ends...
      //#ifdef TESTING
      //std::cerr << "    Coagulating.\n";
      //#endif
      //if( current == f->next ) {
	// Getting rid of current. Scary.
      //	current = 0;
      //}
      f->num+=f->next->num;
      f->next=f->next->next;
    } else {
      f = f->next;
    }
  }
  //#ifdef TESTING
  //std::cerr << "    Start is " << start << " for " << start->num << " until " << start->next << "\n";
  //#endif
  //return ((FreeMarker *)(0))->next;
  //return start;
  first = start;
  current = 0;
}

Locality::Locality( locality_t id ) : m_id(id), m_master( g_pagesize, 0, 0 ), m_scatter( sizeof( SimpleAlloc::FreeMarker ), filemapper_ptr<void>(), 0 ), m_sizes() {
  // This assumes, almost certainly correctly, that sizeof(Locality)<g_pagesize.
  // g_pagesize is, incidentally, almost always 4k.
  std::size_t foo( g_pagesize/sizeof(SimpleAlloc::FreeMarker) );
  std::size_t q( 1+(sizeof(Locality)/sizeof(SimpleAlloc::FreeMarker)) );
  m_scatter.deallocate( filemapper_ptr<SimpleAlloc>( id, q), foo-q );
  m_master.deallocate( filemapper_ptr<SimpleAlloc>( id, g_pagesize ), g_pagesize*1023 )
}

filemapper_ptr<SimpleAlloc> Locality::simplealloc( std::size_t s ) {
  if( s==g_pagesize ) {
    return filemapper_ptr<SimpleAlloc>( m_id, sizeof( m_id ) );
  }
  if( ScopeLocality::current()<0 ) {
    return filemapper_ptr<SimpleAlloc>( m_id, sizeof( m_id )+sizeof( m_master ) );
  }
  Threading::WriteLock l__inst( m_finelock );
  ScopeLocality loc__inst( -id );
  if( m_sizes.find(s)==m_sizes.end() ) {
    // Threading::WriteLock l__inst( m_finelock );
    m_sizes[s] = SimpleAlloc( s, 0, 0 );
  }
  return &m_sizes[s];
}

void Locality::report( std::ostream & o ) {
  Threading::ReadLock l__inst( m_finelock );
  o << "Locality " << m_id << ":\n";
  for( t_sizes::iterator i( m_sizes.begin() ); m_sizes.end()!=i; ++i ) {
    o << "  Size " << (*i).first << " currently at " << (*i).second.m_allocs << std::endl;
    (*i).second.reorder();
  }
  o << "--\n";
}
			     
#ifdef TESTING_SIMPLEALLOC
int main( int argc, char ** argv ) {
  try {
    void * chunk( ::operator new( g_pagesize*10 ) );
    void * heap( reinterpret_cast<void *>( reinterpret_cast<unsigned long>( chunk ) + g_pagesize ) );
    SimpleAlloc * a( reinterpret_cast<SimpleAlloc *>( chunk ) );
    new(a) SimpleAlloc( 32, heap, 9 );
    for( int i(0); i<5000; ++i ) {
      //std::cerr << "Allocation loop start.\n";
      std::list<void *> l;
      for( int i(0); i!=((g_pagesize*8)/64); ++i ) {
	l.push_back( a->allocate( 2 ) );
	a->deallocate( a->allocate( 1 ), 1 );
      }
      //std::cerr << "Deallocation loop start.\n";
      while( !l.empty() ) {
	a->deallocate( (*l.begin()), 2 );
	l.erase( l.begin() );
      }
      //std::cerr << "Should now be clear.\n";
    }
    // a->reorder();
    if( a->allocate( (g_pagesize*9)/32 )==0 ) {
      std::cerr << "Damn, leak.\n";
    }
  } catch( std::exception & e ) {
    std::cerr << "Terminal exception: " << e.what() << std::endl;
  }
  return 0;
}
#endif

#ifdef TESTING_FILEMAPPER
typedef std::basic_string<char, std::char_traits<char>, filemapper<char> > pstring;
typedef std::map< int, pstring, std::less<int>, filemapper<pstring> > pmap;

int main( int argc, char ** argv ) {
  try {
    ::Utils::MMap::MMap m( "storage-file" );
    g_mainheap = &m;
    {
      ScopeLocality loc__inst( 4 );
      pstring * str_p( reinterpret_cast<pstring *>( g_mainheap->bootpointer() ) );
      if( str_p ) {
	std::cout << "The string is: " << *str_p << std::endl;
      } else {
	std::cout << "No string, so allocating a new one." << std::endl;
	filemapper< pstring > alloc;
	str_p = alloc.allocate( 1, 0 );
	// alloc.construct( str_p, pstring() );
	new(str_p) pstring();
	std::cout << "New string is at: " << str_p << std::endl << std::flush;
	std::cout << "String is currently: " << *str_p << std::endl << std::flush;
      }
      pstring & str( *str_p );
      str = "Foo!";
      str = "Wibble!";
      str = "Yowl!";
      str.clear();
      str = "MooooooooooooOO!";
      g_mainheap->bootpointer( str_p );
    }
  } catch( std::exception & e ) {
    std::cerr << " Died with exception: " << e.what() << std::endl;
  }
  return 0;
}
#endif
    
