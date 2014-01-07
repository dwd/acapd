// -*- c++ -*-
#define FDSTREAMBUF_IMPL
#include "fdstreambuf.hh"
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include <cstring>
//#include <iostream>

namespace {
  const int buflen( 2048 );
}

namespace Infotrope {
  namespace Stream {
    
    template < typename C, typename T >
    inline basic_fdstreambuf<C,T>::basic_fdstreambuf()
      : m_fd( -1 ), m_block( true ), m_autoclose( false ), m_buffer( new typename basic_fdstreambuf < C, T >::char_type[ buflen ] ) {}
    
    
    
    template < typename C, typename T >
    inline basic_fdstreambuf<C,T>::basic_fdstreambuf( int fd )
      : m_fd( -1 ), m_block( true ), m_autoclose( false ), m_buffer( new typename basic_fdstreambuf< C, T >::char_type[ buflen ] ) {
        attach( fd );
      }
    
    template < typename C, typename T >
    inline void basic_fdstreambuf<C,T>::attach( int fd ) {
      maybe_close();
      m_fd = fd;
      m_autoclose = false;
      setup();
    }
    
    template < typename C, typename T >
    inline basic_fdstreambuf<C,T>::~basic_fdstreambuf() {
      if( this->eback() ) delete[] this->eback();
      delete[] m_buffer;
    }
    
    template < typename C, typename T >
    typename basic_fdstreambuf<C,T>::int_type basic_fdstreambuf<C,T>::overflow( typename basic_fdstreambuf<C,T>::int_type ch ) {
      int len( this->pptr() - this->pbase() );
      if( !traits_type::eq_int_type( ch, traits_type::eof() ) ) ++len;
      if( len ) {
        char_type * sending( new char_type[ len ] );
        char_type * spos( sending );
        std::memcpy( reinterpret_cast<void *>( sending ), reinterpret_cast<void *>( this->pbase() ), this->pptr() - this->pbase() );
        if ( !traits_type::eq_int_type( ch, traits_type::eof() ) ) sending[ len - 1 ] = ch;
        int written( write( reinterpret_cast<char *>( spos ), len ) );
        if ( written == 0 ) return traits_type::eof();
      }
      this->setp( m_buffer, m_buffer + buflen );
      return 0;
    }
    
    template < typename C, typename T >
    int basic_fdstreambuf<C,T>::sync() {
      int_type t( overflow( traits_type::eof() ) );
      return t;
    }
    
    template < typename C, typename T >
    typename basic_fdstreambuf<C,T>::int_type basic_fdstreambuf<C,T>::underflow() {
      //std::cout << "fdstreambuf::underflow()\r\n";
      char_type * buf( new char_type[ 2048 ] );
      
      int read_len( read( reinterpret_cast < char * > ( buf ), 2048 ) );
      if ( read_len == 0 ) return traits_type::eof();
      if ( this->eback() ) delete[] this->eback();
      
      this->setg( buf, buf, buf + read_len );
      //std::cout << "fdstreambuf::underflow() returning non-EOF.\r\n";
      //std::cout << " - read " << read_len << "bytes, from " << reinterpret_cast<long>(gptr()) << " to " << reinterpret_cast<long>(egptr()) << "\r\n";
      //std::cout << "Returning '" << buf[0] << "' " << static_cast<int>( static_cast<unsigned char>(buf[0]) ) << "\r\n";
      return 0;
    }
    
    template < typename C, typename T >
    int basic_fdstreambuf<C,T>::read( char * ptr, int n ) {
      //std::cout << "Read!\r\n" << std::flush;
      int readen( 0 );   // As in written. English too overloaded. :-)
      if ( m_block ) {
        readen = ::read( m_fd, ptr, n );
        //std::cout << "Non-blocking read of " << readen << "bytes completed.\r\n";
        if ( readen < 0 ) {
          throw Exception::fderror( errno, m_fd, "Blocking read" );
        }
      } else {
        int r;
        while ( ( r = ::read( m_fd, ptr, n ) ) >= 0 ) {
          //std::cout << "Blocking read of " << r << "bytes completed.\r\n" << std::flush;
          readen += r;
          ptr += r;
          n -= r;
          if ( n <= 0 ) {
            break;
          }
        }
        if ( r < 0 ) {
          if ( errno != EAGAIN ) {
            throw Exception::fderror( errno, m_fd, "Non-blocking read" );
          }
        }
      }
      return readen;
    }
    
    template < typename C, typename T >
    int basic_fdstreambuf<C,T>::write( char * ptr, int n ) {
      int written( 0 );
      if ( m_block ) {
        written = ::write( m_fd, ptr, n );
        if ( written < 0 ) {
          throw Exception::fderror( errno, m_fd, "Blocking write" );
        }
      } else {
        int r;
        while ( ( r = ::write( m_fd, ptr, n ) ) >= 0 ) {
          written += r;
          ptr += r;
          n -= r;
          if ( n <= 0 ) {
            break;
          }
        }
        if ( r < 0 ) {
          if ( errno != EAGAIN ) {
            throw Exception::fderror( errno, m_fd, "Non-blocking write" );
          }
        }
      }
      return written;
    }
    
    template < typename C, typename T >
    void basic_fdstreambuf<C,T>::maybe_close() {
      //std::cout << "12345 Maybe close?\n";
      sync();
      if( m_autoclose ) {
        //std::cout << "12345 - Yeah, really close.\n";
        close();
      }
    }
    
    template < typename C, typename T >
    void basic_fdstreambuf<C,T>::close() {
      //std::cout << "12345 - fdstreambuf closing.\n" << std::flush;
      sync();
      if( m_fd >= 0 ) {
        really_close();
      }
    }
    
    template < typename C, typename T >
    void basic_fdstreambuf<C,T>::really_close() {
      //std::cout << "12345 - fdstreambuf actual close.\n" << std::flush;
      ::close( m_fd );
    }
    
    template < typename C, typename T >
    void basic_fdstreambuf<C,T>::setup() {
      this->setp( m_buffer, m_buffer + buflen );
    }
    
    template < typename C, typename T >
    long unsigned basic_fdstreambuf<C,T>::bits() const {
      return 0;
    }
  }
}
