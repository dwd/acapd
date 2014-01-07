#ifndef INFOTROPE_UTILS_FILE_MAPPER_PTR_HH
#define INFOTROPE_UTILS_FILE_MAPPER_PTR_HH

#include <infotrope/utils/file-mapper-base.hh>
#include <bits/stl_iterator_base_types.h>

namespace Infotrope {
  
  namespace Utils {
    
    namespace MMap {
      
      template< typename T> class filemapper_ptr {
      public:
	std::size_t m_offset;
	locality_type m_locality;
	typedef T value_type;
	
      public:
	filemapper_ptr( std::size_t offset, locality_type locality )
	  : m_offset( offset ), m_locality( locality ) {
	  }
	
	explicit filemapper_ptr( std::size_t i )
	  : m_offset( 0 ), m_locality( 0 ) {
	  }
	
	filemapper_ptr()
	  : m_offset( 0 ), m_locality( 0 ) {
	  }
	
	//explicit filemapper_ptr( filemapper_ptr<void> const & o )
	//: m_offset( o.m_offset ), m_locality( o.m_locality ) {
	//}
	
	filemapper_ptr( filemapper_ptr<T> const & o )
	  : m_offset( o.m_offset ), m_locality( o.m_locality ) {
	  }
	
	filemapper_ptr<T> & operator++() {
	  m_offset += sizeof(T);
	  return *this;
	}
	
	T * const ptr() {
	  return reinterpret_cast<T*>( m_offset + g_mmap_man->pointer( m_locality ) );
	}
	T const * const ptr() const {
	  return reinterpret_cast<T const *>( m_offset + g_mmap_man->pointer( m_locality ) );
	}
	
	T & operator*() const {
	  return *ptr();
	}
	T * const operator->() const {
	  return ptr();
	}
	T & operator*() {
	  return *ptr();
	}
	T * const operator->() {
	  return ptr();
	}
	
	bool operator==( filemapper_ptr<T> const & o ) const {
	  return (m_locality==o.m_locality)&&(m_offset==o.m_offset);
	}
	bool operator!=( filemapper_ptr<T> const & o ) const {
	  return (m_locality!=o.m_locality)||(m_offset!=o.m_offset);
	}
	bool operator <( filemapper_ptr<T> const & o ) const {
	  if( m_locality<o.m_locality ) return true;
	  if( m_locality>o.m_locality ) return false;
	  return m_offset<o.m_offset;
	}
	operator bool() const {
	  // Note that the offset is allowed to be zero,
	  // but that the locality can never be.
	  // A non-zero offset with a zero locality is an error.
	  return m_locality;
	}
	bool operator==(int) const {
	  // Comparison with an int suggests comparison with a 0.
	  return *this;
	}
	filemapper_ptr<T> operator+( int x ) {
	  return filemapper_ptr<T>( m_offset + (x*sizeof(T)), m_locality );
	}
	filemapper_ptr<T> operator-( int x ) {
	  return this->operator+( -x );
	}
	T & operator[]( int x ) {
	  return *((*this)+x);
	}
	
	filemapper_ptr<T> & operator=( filemapper_ptr<T> const & t ) {
	  m_offset = t.m_offset;
	  m_locality = t.m_locality;
	  return *this;
	}
	filemapper_ptr<T> & operator=( int ) {
	  m_locality = 0;
	  m_offset = 0;
	  return *this;
	}
	
	template<typename X> struct rebind {
	  typedef filemapper_ptr<X> other;
	  static other & reinterpret( filemapper_ptr<T> & p ) {
	    return *reinterpret_cast<other *>( &p );
	  }
	  static other const & reinterpret( filemapper_ptr<T> const & p ) {
	    return *reinterpret_cast<other const *>( &p );
	  }
	};
	
      };
      
      template<> class filemapper_ptr<void> {
      public:
	std::size_t m_offset;
	locality_type m_locality;
	typedef void value_type;
	
      public:
	filemapper_ptr( std::size_t offset, locality_type locality )
	  : m_offset( offset ), m_locality( locality ) {
	  }
	
	explicit filemapper_ptr( std::size_t i )
	  : m_offset( 0 ), m_locality( 0 ) {
	  }
	
	filemapper_ptr()
	  : m_offset( 0 ), m_locality( 0 ) {
	  }
	
	template<typename X> filemapper_ptr( filemapper_ptr<X> const & o )
	  : m_offset( o.m_offset ), m_locality( o.m_locality ) {
	  }
	
	template<typename X> bool operator==( filemapper_ptr<X> const & o ) const {
	  return (m_locality==o.m_locality)&&(m_offset==o.m_offset);
	}
	template<typename X> bool operator!=( filemapper_ptr<X> const & o ) const {
	  return (m_locality!=o.m_locality)||(m_offset!=o.m_offset);
	}
	template<typename X> bool operator <( filemapper_ptr<X> const & o ) const {
	  if( m_locality<o.m_locality ) return true;
	  if( m_locality>o.m_locality ) return false;
	  return m_offset<o.m_offset;
	}
	operator bool() const {
	  // Note that the offset is allowed to be zero,
	  // but that the locality can never be.
	  // A non-zero offset with a zero locality is an error.
	  return m_locality;
	}
	bool operator==(int) const {
	  // Comparison with an int suggests comparison with a 0.
	  return *this;
	}
	
	template<typename X> filemapper_ptr<void> & operator=( filemapper_ptr<X> const & t ) {
	  m_offset = t.m_offset;
	  m_locality = t.m_locality;
	  return *this;
	}
	filemapper_ptr<void> & operator=( int ) {
	  m_locality = 0;
	  m_offset = 0;
	  return *this;
	}
	
	template<typename X> struct rebind {
	  typedef filemapper_ptr<X> other;
	  static other & reinterpret( filemapper_ptr<void> & p ) {
	    return *reinterpret_cast<other *>( &p );
	  }
	  static other const & reinterpret( filemapper_ptr<void> const & p ) {
	    return *reinterpret_cast<other const *>( &p );
	  }
	};
	
      };
      
    }
  }
}

namespace std {
  template<typename _Tp>
  struct iterator_traits< Infotrope::Utils::MMap::filemapper_ptr<_Tp> > {
    typedef random_access_iterator_tag iterator_category;
    typedef _Tp                         value_type;
    typedef ptrdiff_t                   difference_type;
    typedef _Tp * const                        pointer;
    typedef _Tp &                        reference;
  };
  
  template<typename _Tp>
  struct iterator_traits< Infotrope::Utils::MMap::filemapper_ptr<_Tp> const > {
    typedef random_access_iterator_tag iterator_category;
    typedef _Tp                         value_type;
    typedef ptrdiff_t                   difference_type;
    typedef const _Tp * const                  pointer;
    typedef const _Tp &                  reference;
  };
}

#endif
