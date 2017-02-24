// -*- c++ -*-
#ifndef INFOTROPE_TCPSTREAM_TCC
#define INFOTROPE_TCPSTREAM_TCC
#include <infotrope/config.h>
#include <infotrope/stream/tcpstream.hh>
#include <infotrope/stream/fdstreambuf.hh>
#ifdef INFOTROPE_HAVE_TLS
#include <infotrope/stream/tlsstreambuf.hh>
#endif

#include <sys/types.h> // socket()
#include <sys/socket.h> // socket()
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

namespace Infotrope {
  
  namespace Stream {
    
    template < typename C, typename T > inline basic_tcpstream< C, T >::basic_tcpstream( int fd )
      : std::basic_iostream< C, T >( new basic_fdstreambuf< C, T >( fd ) ), m_bits(0) {
      }
    
    
    
    template < typename C, typename T > inline basic_tcpstream < C, T > ::basic_tcpstream()
      : std::basic_iostream < C, T > ( new fdstreambuf() ), m_bits(0) {
      }
    
    template<typename C, typename T> inline void basic_tcpstream<C,T>::autoclose( bool t ) {
      dynamic_cast< basic_fdstreambuf<C,T> * >( this->rdbuf() )->autoclose( t );
    }
    
    template<typename C, typename T> inline bool basic_tcpstream<C,T>::autoclose() const {
      return dynamic_cast< basic_fdstreambuf<C,T> * >( this->rdbuf() )->autoclose();
    }
    
    template < typename C, typename T > inline void basic_tcpstream < C, T > ::connect( std::string const & host, unsigned short int port ) {
      int fd( socket( AF_INET, SOCK_STREAM, 0 ) );
      if ( fd < 0 ) {
	throw Exception::fderror( errno, fd, "socket()" );
      }
      struct sockaddr_in sin;
      sin.sin_family = AF_INET;
      hostent * hoste = gethostbyname( host.c_str() );
      if ( !hoste ) {
	throw std::string( "Unknown host : " + host );
      }
      memcpy( &sin.sin_addr.s_addr, hoste->h_addr_list[ 0 ], 4 );
      sin.sin_port = htons( port );
      if ( 0 > ::connect( fd, reinterpret_cast < struct sockaddr * > ( &sin ), sizeof( struct sockaddr_in ) ) ) {
	throw Exception::fderror( errno, fd, "connect()" );
      }
      dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->attach( fd );
      dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->autoclose( true );
    }
    
#ifdef INFOTROPE_HAVE_TLS
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls() {
      return starttls( new basic_tlsstreambuf < C, T > ( false, dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ) );
    }
    
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls( bool server, std::string const & certfile ) {
      return starttls( new basic_tlsstreambuf < C, T > ( server, dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ), certfile ) );
    }
    
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls( basic_tlsstreambuf < C, T > * tls ) {
      basic_fdstreambuf < C, T > * fd = dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() );
      this->rdbuf( tls );
      if ( tls->starttls() ) {
	fd->autoclose( false );
	delete fd;
	m_bits = tls->bits();
	return true;
      } else {
	tls->autoclose( false );
	delete tls;   // Memory leak, I think.
	this->rdbuf( fd );
	return false;
      }
    }
#else
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls() {
      return false;
    }
    
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls( bool server, std::string const & certfile ) {
      return false;
    }
    
    // Just a dummy, in the hope it maintains some binary compatibility or something.
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::starttls( void * tls ) {
      return false;
    }
#endif  
    
#ifdef INFOTROPE_HAVE_SASL_SASL_H
    template < typename C, typename T > inline bool basic_tcpstream<C,T>::startsasl( sasl_conn_t * s ) {
      basic_fdstreambuf<C,T> * fd( dynamic_cast<basic_fdstreambuf<C,T>*>( this->rdbuf() ) );
#ifdef INFOTROPE_HAVE_ZLIB_H
      basic_deflatestreambuf<C,T> * zfd( dynamic_cast<basic_deflatestreambuf<C,T>*>( fd ) );
      if(zfd) {
	  fd = zfd->slave();
      }
#endif
      basic_saslstreambuf<C,T> * sasl( new basic_saslstreambuf<C,T>( s, fd ) );
      m_bits += sasl->bits();
      fd->autoclose( false );
#ifdef INFOTROPE_HAVE_ZLIB_H
      if(zfd) {
	  zfd->slave(sasl);
      } else {
#endif
	  this->rdbuf( sasl );
#ifdef INFOTROPE_HAVE_ZLIB_H
      }
#endif
      return true;
    }
#else
    template < typename C, typename T > inline bool basic_tcpstream<C,T>::startsasl( void * s ) {
      return false;
    }
#endif

#ifdef INFOTROPE_HAVE_ZLIB_H
    template < typename C, typename T > inline bool basic_tcpstream<C,T>::startdeflate() {
      basic_fdstreambuf<C,T> * fd( dynamic_cast<basic_fdstreambuf<C,T>*>( this->rdbuf() ) );
      basic_deflatestreambuf<C,T> * zlib( new basic_deflatestreambuf<C,T>( fd ) );
      m_bits = zlib->bits();
      fd->autoclose( false );
      this->rdbuf( zlib );
      return true;
    }
#endif
    
    template < typename C, typename T > inline bool basic_tcpstream < C, T > ::connected() {
      return ( dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->fd() >= 0 );
    }
    
    template < typename C, typename T > inline void basic_tcpstream < C, T > ::attach( int fd ) {
      dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->attach( fd );
      dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->autoclose( true );
    }
    
    template < typename C, typename T > inline basic_tcpstream < C, T > ::~basic_tcpstream() {
      if( this->rdbuf() ) {
	dynamic_cast< basic_fdstreambuf<C,T> * >( this->rdbuf() )->maybe_close();
	delete this->rdbuf();
	this->rdbuf( 0 );
      }
    }
    
    template < typename C, typename T > inline int basic_tcpstream < C, T > ::fd() const {
      return dynamic_cast < basic_fdstreambuf < C, T > * > ( this->rdbuf() ) ->fd();
    }
    
    template < typename C, typename T > inline unsigned long int basic_tcpstream<C,T>::bits() const {
      return m_bits;
    }
  }
}

#endif