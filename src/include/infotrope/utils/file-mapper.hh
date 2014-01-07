#ifndef FILEMAPPER_HH
#define FILEMAPPER_HH
#define SIMPLEALLOC_TRACK

#include <memory>
#include <map>
#include <set>
#include <ostream>

#ifdef _REENTRANT
#include <infotrope/threading/rw-lock.hh>
#endif

#include <infotrope/utils/file-mapper-ptr.hh>

namespace Infotrope {
  
  namespace Utils {
    
    namespace MMap {
      
      class Header;
      class HeapHeader;
      class MainHeap;
      class SimpleAlloc;
      class FlexAlloc;
      
      template<typename Tp,typename Mp=SimpleAlloc> class filemapper;
      
      class Locality;
      class Extent;
      extern const int g_pagesize;
      
      // Yeah, we're borrowing the C++ scope to use as our locality stack.
      // Neat or *what*?
      class ScopeLocality {
      public:
	locality_type m_old;
#ifdef _REENTRANT
	static pthread_key_t s_current_key;
	static pthread_once_t s_once;
	static void key_create() {
	  pthread_key_create( &s_current_key, NULL );
	}
	
	ScopeLocality( locality_type s ) {
	  pthread_once( &s_once, key_create );
	  m_old = reinterpret_cast<locality_type>( pthread_getspecific( s_current_key ) );
	  pthread_setspecific( s_current_key, reinterpret_cast<void *>( s ) );
	}
	
	static locality_type current() {
	  return reinterpret_cast<locality_type>( pthread_getspecific( s_current_key ) );
	}
	
	~ScopeLocality() {
	  pthread_setspecific( s_current_key, reinterpret_cast<void *>( m_old ) );
	}
#else
	static locality_type s_current;
	ScopeLocality( locality_type s ) : m_old( s_current ) {
	  s_current = s;
	}
	
	static locality_type current() {
	  return s_current;
	}
	
	~ScopeLocality() {
	  s_current = m_old;
	}
#endif
      };
      
      // We use these to allocate and free without allocations.
      // They're basically pretty trivial things, operating via linked lists.
      // Reduction of sequential blocks of free space is only done
      // at exhausted allocation time, hence this is relatively slow
      // for larger allocations than "normal".
      // However, when the number of units changes rarely, this should be pretty
      // fast, since it'll reuse space without having to coagulate.
      // In short, excellent for maps and lists, not so good for vectors.
      class SimpleAlloc {
      private:
	std::size_t m_size;
      public:
	struct FreeMarker {
	  filemapper_ptr<FreeMarker> next;
	  std::size_t num;
	  filemapper_ptr<FreeMarker> glueme( std::size_t s );
	  filemapper_ptr<FreeMarker> trynext( std::size_t n );
	};
      private:
	filemapper_ptr<FreeMarker> first;
	filemapper_ptr<FreeMarker> current;
	filemapper_ptr<FreeMarker> end;
#ifdef _REENTRANT
	Threading::RWMutex m_finelock;
	// Actually only locked for write, but this is recursive...
#endif
	
      public:
#ifdef SIMPLEALLOC_TRACK
	signed long int m_allocs;
#endif
	SimpleAlloc(); // Non-initializing.
	SimpleAlloc( std::size_t s, filemapper_ptr<void> p, std::size_t n ); // Initializing.
	
	filemapper_ptr<void> allocate( std::size_t n, signed long int l, int pass=0 ); // Allocate n units of m_size
	filemapper_ptr<FreeMarker> allocate( filemapper_ptr<FreeMarker>, std::size_t ); // Allocate at this point, return new FreeMarker.
	void exhausted( std::size_t n, signed long int l ); // Called when allocate can't find any memory even after reordering etc.
	void reorder();
	void deallocate( filemapper_ptr<void>, std::size_t n ); // Free n units of m_size starting at p
	std::size_t size() const {
	  return m_size;
	}
      };
      
      // A best-fit allocator, driven by std::map.
      // Suitable for random size allocations. (Although you can still use SimpleAlloc for these.)
      // It's O(log(n)) complexity, n being the number of current allocations.
      class FlexAlloc {
      };
      
      template<typename Alloc> filemapper_ptr<Alloc> get_allocator( std::size_t, locality_type ) {
	return filemapper<void>();
      }
      
      template<> filemapper_ptr<SimpleAlloc> get_allocator<SimpleAlloc>( std::size_t x, locality_type y ) {
	return g_mmap_man->simplealloc( x, y );
      }
      
      // STL allocator, parametrizable to either SimpleAlloc or FlexAlloc.
      template< typename Tp, typename Mp > class filemapper {
      public:
	typedef Mp master_allocator;
      public:
	signed long int m_id;
	filemapper_ptr<master_allocator> m_master;
      public:
	typedef Tp value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef filemapper_ptr<value_type> pointer;
	typedef filemapper_ptr<value_type> const const_pointer;
	typedef value_type & reference;
	typedef value_type const & const_reference;
	
	/*pointer address( reference t ) const {
	  return &t;
	  }
	  const_pointer address( const_reference t ) const {
	  return &t;
	  }*/
	
	
	//filemapper( signed long int id ) : m_id( id ), m_master( 0 ) {
	//}
	
	filemapper() : m_id( ScopeLocality::current() ), m_master( 0 ) {
	}
	
	filemapper( filemapper<Tp,Mp> const & f ) : m_id( f.m_id ), m_master( 0 ) {
	}
	
	template< typename O, typename Q > filemapper( filemapper<O,Q> const & f ) : m_id( f.m_id ), m_master( 0 ) {
	}
	
	pointer allocate( std::size_t n, void * hint=0 ) {
	  if( !m_master ) {
	    std::size_t n2( sizeof( SimpleAlloc::FreeMarker ) );
	    while( n2<sizeof(value_type) ) n2+=sizeof(int);
	    m_master = ::Infotrope::Utils::MMap::get_allocator<master_allocator>( n2, m_id );
	  }
	  pointer p( m_master->allocate( (n*sizeof(value_type)+m_master->size()-1)/m_master->size(), m_id ) );
	  // g_alloc_map[alloc_range( p, n*sizeof(value_type) )] = m_id;
	  //std::cout << g_alloc_map[p] << " " << g_alloc_map[p+1] << std::endl;
	  if( !p ) {
	    throw std::bad_alloc();
	  }
	  return p;
	}
	
	void deallocate( pointer p, std::size_t n  ) {
	  if( !p ) return;
	  std::size_t n2( sizeof( SimpleAlloc::FreeMarker ) );
	  while( n2<sizeof(value_type) ) n2+=sizeof(int);
	  //std::cerr << "Deallocating " << reinterpret_cast<void *>(p) << " for " << n2 << " of " << ((n*sizeof(value_type)+n2-1)/n2) << " in locality " << g_mainheap->locality(p) << std::endl;
	  filemapper_ptr<master_allocator> alloc( ::Infotrope::Utils::MMap::get_allocator<master_allocator>( n2, p.m_locality ) );
	  //std::cout << "  Deallocating " << n << " units with id " << m_id << std::endl;
	  // g_alloc_map[alloc_range( p, n*sizeof(value_type) )] = 0;
	  alloc->deallocate( pointer::rebind<void>::reinterpret(p), (n*sizeof(value_type)+alloc->size()-1)/alloc->size() );
	  //delete[] reinterpret_cast<char *>( p );
	}
	
	void construct( pointer p, const_reference t ) {
	  //std::cout << "  Construction in progress for id " << m_id << std::endl;
	  new(p.ptr()) value_type(t);
	  //std::cout << "  Construction complete for id " << m_id << std::endl;
	}
	
	void destroy( pointer p ) {
	  //std::cout << "  Destruction in progress for id " << m_id << std::endl;
	  if(p.ptr()) p->~value_type();
	  //std::cout << "  Destruction complete for id " << m_id << std::endl;
	}
	
	size_type max_size( void ) const throw() {
	  return ((static_cast<std::size_t>( -1 ))/sizeof(value_type));
	}
	
	template< typename O > struct rebind {
	  typedef filemapper<O,master_allocator> other;
	};
	
	bool operator == ( filemapper<Tp,master_allocator> const & f ) const {
	  //return true;
	  return f.m_id==m_id;
	}
	bool operator != ( filemapper<Tp,master_allocator> const & f ) const {
	  //return false;
	  return f.m_id!=m_id;
	}
	signed long int id() const {
	  return m_id;
	}
      };
      
      // void specialization.
      template<typename Mp> class filemapper<void,Mp> {
      public:
	typedef Mp master_allocator;
      public:
	signed long int m_id;
	filemapper_ptr<master_allocator> m_master;
      public:
	typedef void value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef filemapper_ptr<value_type> pointer;
	typedef filemapper_ptr<value_type> const const_pointer;
	
	filemapper() : m_id( ScopeLocality::current() ), m_master( 0 ) {
	}
	
	filemapper( filemapper<void,Mp> const & f ) : m_id( f.m_id ), m_master( 0 ) {
	}
	
	template< typename O, typename Q > filemapper( filemapper<O,Q> const & f ) : m_id( f.m_id ), m_master( 0 ) {
	}
	
	pointer allocate( std::size_t n, void * hint=0 ) {
	  throw std::bad_alloc();
	}
	
	size_type max_size( void ) const throw() {
	  return (static_cast<std::size_t>( -1 ));
	}
	
	template< typename O > struct rebind {
	  typedef filemapper<O,master_allocator> other;
	};
	
	bool operator == ( filemapper<void,master_allocator> const & f ) const {
	  //return true;
	  return f.m_id==m_id;
	}
	bool operator != ( filemapper<void,master_allocator> const & f ) const {
	  //return false;
	  return f.m_id!=m_id;
	}
	signed long int id() const {
	  return m_id;
	}
      };
      
      class Locality {
      private:
	//std::set< Extent *, ExtentInside, filemapper<Extent*> > m_extents;
	signed long int m_id;
	//Extent * m_next;
	SimpleAlloc m_master; // Page allocator.
	SimpleAlloc m_scatter; // Miniture Allocator, for anything where we don't yet know the size. Typically, the following:
	typedef std::map< std::size_t, filemapper_ptr<SimpleAlloc>, std::less<std::size_t>, filemapper< std::pair<std::size_t,filemapper_ptr<SimpleAlloc> > > > t_sizes;
	t_sizes m_sizes;
#ifdef _REENTRANT
	Infotrope::Threading::RWMutex m_finelock;
#endif
	
      public:
	Locality();
	Locality( signed long int );
	filemapper_ptr<SimpleAlloc> simplealloc( std::size_t );
	int id() const {
	  return m_id;
	}
	void report( std::ostream & );
      };
      
      // Read in from the file, stored at beginning of mmap.
      class HeapHeader {
      public:
	void * load_ptr; // Needs to be first.
	bool checkpoint; // Our last sync was at a checkpoint.
	SimpleAlloc page_alloc; // Base allocator for pages.
	bool locality_sane;
	SimpleAlloc locality_alloc; // Base allocator for the locality map.
	bool extent_sane;
	SimpleAlloc extent_alloc; // Base allocator for the extent map.
	typedef std::set< Extent,std::less<Extent>,filemapper<Extent> > t_extents;
	t_extents extents;
	typedef std::map< int, Locality, std::less<int>, filemapper<Locality> > t_localities;
	t_localities localities;
	filemapper_ptr<void> bootpointer;
      };
      
    }
    
  }

}
#endif
