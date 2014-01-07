// -*- c++ -*-
#include <infotrope/stream/deflatestreambuf.hh>
#include <zlib.h>

//#include <iostream>

#include <cstring>

//#define INFOTROPE_DEFLATE_DEBUG

#ifdef INFOTROPE_DEFLATE_DEBUG
#include <infotrope/server/log.hh>
#include <sstream>
#endif

namespace Infotrope {
  namespace Stream {
    
    const unsigned int zlib_buffer_size( 16384 );
#ifdef INFOTROPE_DEFLATE_DEBUG
    inline void log( std::string const & s, int i ) {
      std::ostringstream ss;
      ss << "zlib : " << s << " returned " << i;
      Infotrope::Server::Log( 5, ss.str() );
    }
#else
    inline void log( std::string const &, int ) {}
#endif
    
    template < typename C, typename T >
    inline basic_deflatestreambuf< C, T >::basic_deflatestreambuf( basic_fdstreambuf< C, T > * b )
      : basic_fdstreambuf< C, T >( b->fd() ), m_slave( b ) {
      this->autoclose( m_slave->autoclose() );
      m_slave->autoclose( false );
      this->blocking( m_slave->blocking() );
      m_buffer = new unsigned char[zlib_buffer_size];
      m_inbuffer = new unsigned char[zlib_buffer_size];
      std::memset( &m_zlib_out, 0, sizeof(z_stream) );
      std::memset( &m_zlib_in, 0, sizeof(z_stream) );
      m_zlib_in.next_in = m_inbuffer;
      m_zlib_in.avail_in = 0;
      log( "deflateInit2", deflateInit2( &m_zlib_out, 9, 8, -15, 9, 0 ) );
      log( "inflateInit2", inflateInit2( &m_zlib_in, -15 ) );
      m_slave->sync();
    }
    
    template < typename C, typename T >
    inline basic_deflatestreambuf< C, T >::~basic_deflatestreambuf< C, T >() {
      deflateEnd( &m_zlib_out );
      inflateEnd( &m_zlib_in );
      delete[] m_buffer;
      delete[] m_inbuffer;
      delete m_slave;
      m_slave = 0;
    }

      template<typename C, typename T>
      inline basic_fdstreambuf<C,T> * basic_deflatestreambuf<C,T>::slave(basic_fdstreambuf<C,T> * s) {
	  if( !s ) {
	      return m_slave;
	  }
	  sync();
	  basic_fdstreambuf<C,T> * tmp = m_slave;
	  m_slave = s;
	  s->sync();
	  return tmp;
      }
      
    template < typename C, typename T >
    int basic_deflatestreambuf< C, T >::write( char * ptr, int n ) {
      m_zlib_out.next_out = m_buffer;
      m_zlib_out.avail_out = zlib_buffer_size;
      m_zlib_out.next_in = reinterpret_cast<unsigned char *>( ptr );
      m_zlib_out.avail_in = n;
      for(;;) {
	int r( deflate( &m_zlib_out, Z_SYNC_FLUSH ) );
	log( "deflate", r );
	int l( zlib_buffer_size - m_zlib_out.avail_out );
	unsigned char * p( m_buffer );
	while( l > 0 ) {
	  int tmp( m_slave->write( reinterpret_cast<char *>( p ), l ) );
	  p += tmp;
	  l -= tmp;
	  log( "proxy write", tmp );
	}
	if( m_zlib_out.avail_in == 0 ) {
	  return n;
	}
      }
    }
    
    template < typename C, typename T >
    int basic_deflatestreambuf < C, T >::read( char * ptr, int n ) {
      log( "read attempt", n );
      for( ;; ) {
	if( m_inbuffer != m_zlib_in.next_in ) {
	  std::memmove( m_inbuffer, m_zlib_in.next_in, zlib_buffer_size - m_zlib_in.avail_in );
	  m_zlib_in.next_in = m_inbuffer;
	}
	if( m_zlib_in.avail_in < zlib_buffer_size ) {
	  log( "Proxy read", zlib_buffer_size - m_zlib_in.avail_in );
	  int tmp( m_slave->read( reinterpret_cast<char *>( m_zlib_in.next_in + m_zlib_in.avail_in ), zlib_buffer_size - m_zlib_in.avail_in ) );
	  m_zlib_in.avail_in += tmp;
	  log( "Proxiy read done,", tmp );
	}
	m_zlib_in.next_out = reinterpret_cast<unsigned char *>( ptr );
	m_zlib_in.avail_out = n;
	log( "avail_in", m_zlib_in.avail_in );
	int r( inflate( &m_zlib_in, 0 ) );
	log( "inflate", r );
	if( r != 0 || m_zlib_in.avail_in == 0 ) {
	  log( "read", n - m_zlib_in.avail_out );
	  return n - m_zlib_in.avail_out;
	}
      }
    }
    
    template < typename C, typename T >
    inline long unsigned basic_deflatestreambuf<C,T>::bits() const {
      return 0;
    }
  }
}
