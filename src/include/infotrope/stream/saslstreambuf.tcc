// -*- c++ -*-
#include <infotrope/stream/saslstreambuf.hh>
extern "C" {
#include <sasl/sasl.h>
}

//#include <iostream>

namespace Infotrope {
  namespace Stream {
    template < typename C, typename T >
    inline basic_saslstreambuf< C, T >::basic_saslstreambuf( sasl_conn_t * sasl, basic_fdstreambuf< C, T > * b )
      : basic_fdstreambuf< C, T >( b->fd() ), m_sasl_conn( sasl ), m_maxoutbuf(0), m_slave( b ) {
      const unsigned * tmp;
      if( SASL_OK==sasl_getprop( m_sasl_conn, SASL_MAXOUTBUF, reinterpret_cast<const void **>( &tmp ) ) ) {
        m_maxoutbuf = *tmp;
      } else {
        throw std::string( "SASL getprop : " ) + sasl_errdetail( m_sasl_conn );
      }
      this->autoclose( m_slave->autoclose() );
      m_slave->autoclose( false );
      this->blocking( m_slave->blocking() );
    }
    
    template < typename C, typename T >
    inline basic_saslstreambuf< C, T >::~basic_saslstreambuf< C, T >() {
      delete m_slave;
      m_slave = 0;
    }
    
    template < typename C, typename T >
    int basic_saslstreambuf< C, T >::write( char * ptr, int n ) {
      // std::cout << "Writing " << n << " characters.\n" << std::flush;
      // int written( 0 );
      while( static_cast<unsigned>(n)>m_maxoutbuf ) {
        // std::cout << "Recursive reduction of buffer.\n" << std::flush;
        int tmp( write( ptr, m_maxoutbuf ) );
        if( !tmp ) return 0;
        ptr+=tmp;
        n-=tmp;
      }
      // std::cout << "Really writing, now.\n" << std::flush;
      char * buf;
      unsigned len;
      if( SASL_OK==sasl_encode( m_sasl_conn, ptr, n, const_cast<const char **>(&buf), &len ) ) {
        // std::cout << "Encoded to " << len << " bytes.\n" << std::endl;
        while( len>0 ) {
          // std::cout << "Writing to socket.\n" << std::endl;
          int tmp( m_slave->write( buf, len ) );
          buf+=tmp;
          len-=tmp;
          // std::cout << "Wrote " << tmp << " bytes.\n" << std::flush;
        }
        return n;
      } else {
        throw std::string( "SASL encode : " ) + sasl_errdetail( m_sasl_conn );
      }
    }
    
    template < typename C, typename T >
    int basic_saslstreambuf < C, T >::read( char * ptr, int n ) {
      char * buf( new char[n] );
      int tmp( m_slave->read( buf, n/2 ) );
      if( !tmp ) {
        return tmp;
      }
      const char * out;
      unsigned outlen;
      if( SASL_OK==sasl_decode( m_sasl_conn, buf, tmp, &out, &outlen ) ) {
        const char * outstop( out+outlen );
        delete[] buf;
        if( outlen==0 ) {
          // Recurse.
          return read( ptr, n );
        }
        if( outlen > static_cast<unsigned>(n) ) {
          throw std::string( "SASL decode internal : Data supplied exceeded buffer space." );
        }
        while( out<outstop ) *ptr++=*out++;
        return outlen;
      }
      delete[] buf;
      throw std::string( "SASL decode : " ) + sasl_errdetail( m_sasl_conn );
    }
    
    template < typename C, typename T >
    inline long unsigned basic_saslstreambuf<C,T>::bits() const {
      const unsigned *m;
      unsigned b( 0 );
      if( SASL_OK==sasl_getprop( m_sasl_conn, SASL_SSF, reinterpret_cast<const void**>( &m ) ) ) {
        b = *m;
      }
      /*if( m_slave->bits() > b ) {
        b = m_slave->bits();
        }*/
      return b;
    }
  }
}
