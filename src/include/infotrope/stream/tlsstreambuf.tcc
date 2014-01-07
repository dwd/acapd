// -*- c++ -*-
#include <infotrope/stream/tlsstreambuf.hh>
#define INFOTROPE_HAVE_OPENSSL_CRYPTO_H
#include <infotrope/config.h>
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
#include <gnutls/gnutls.h>
#else
#error "No SSL support, I shouldn't be being built."
#endif
#endif
#include <errno.h>
#include <infotrope/stream/tlsstate.hh>

namespace Infotrope {
  namespace Stream {
    template < typename C, typename T >
    inline basic_tlsstreambuf<C,T>::basic_tlsstreambuf( bool server, basic_fdstreambuf < C, T > * b ) : basic_fdstreambuf < C, T > ( b->fd() ), m_tls_state(), m_server( server ), m_cert() {
    }
    
    template < typename C, typename T >
    inline basic_tlsstreambuf<C,T>::basic_tlsstreambuf( bool server, int fd ) : basic_fdstreambuf < C, T > ( fd ), m_tls_state(), m_server( server ), m_cert() {
    }
    
    template < typename C, typename T >
    basic_tlsstreambuf < C, T > ::basic_tlsstreambuf( bool server, basic_fdstreambuf < C, T > * b, std::string const & cert ) : basic_fdstreambuf < C, T > ( b->fd() ), m_tls_state(), m_server( server ), m_cert( cert ) {
    }
    
    template < typename C, typename T >
    inline basic_tlsstreambuf < C, T > ::basic_tlsstreambuf( bool server, int fd, std::string const & cert ) : basic_fdstreambuf < C, T > ( fd ), m_tls_state(), m_server( server ), m_cert( cert ) {
    }
    
    template < typename C, typename T >
    void basic_tlsstreambuf< C, T >::setup() {
      m_tls_state = new tls_state( m_server, m_cert );
      /*
      SSLeay_add_ssl_algorithms();
      if( m_server ) {
	m_tls_state->method = TLSv1_server_method();
      } else {
	m_tls_state->method = TLSv1_client_method();
      }
      SSL_load_error_strings();
      m_tls_state->ctx = SSL_CTX_new( m_tls_state->method );
      if( m_cert.length() ) {
	SSL_CTX_use_certificate_file( m_tls_state->ctx, m_cert.c_str(), SSL_FILETYPE_PEM );
	SSL_CTX_use_PrivateKey_file( m_tls_state->ctx, m_cert.c_str(), SSL_FILETYPE_PEM );
      }
      */
      this->autoclose( true );
      // fdstreambuf::setup(); already called. Confused.
    }
    
    template < typename C, typename T >
    int basic_tlsstreambuf < C, T > ::write( char * ptr, int n ) {
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
      int ret( 0 );
      ret = SSL_write( m_tls_state->ssl, ptr, n );
      int tmp;
      switch ( tmp = SSL_get_error( m_tls_state->ssl, ret ) ) {
      case SSL_ERROR_NONE:
	return ret;
      case SSL_ERROR_ZERO_RETURN:
	return 0;
      case SSL_ERROR_SYSCALL:
	if ( errno == 0 ) {
	  return 0;
	} else {
	  throw new Exception::fderror( errno, this->fd(), "TLS write" );
	}
      default:
	throw std::string( "Unknown SSL error in write. Somewhere." );
      }
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      int tmp( gnutls_record_send( m_tls_state->session, ptr, n ) );
      if( tmp==GNUTLS_E_INTERRUPTED || tmp==GNUTLS_E_AGAIN ) {
	return write( ptr, n );
      } else if( tmp < 0 ) {
	throw Exception::fderror( tmp, this->fd(), "GNUTLS send" );
      }
      return tmp;
      
#endif
#endif
    }
    
    template < typename C, typename T >
    int basic_tlsstreambuf < C, T > ::read( char * ptr, int n ) {
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
      int ret( SSL_read( m_tls_state->ssl, ptr, n ) );
      int tmp;
      switch ( tmp = SSL_get_error( m_tls_state->ssl, ret ) ) {
      case SSL_ERROR_NONE:
	return ret;
      case SSL_ERROR_ZERO_RETURN:
	return 0;
      case SSL_ERROR_SYSCALL:
	if ( errno == 0 ) {
	  return 0;
	} else {
	  throw new Exception::fderror( errno, this->fd(), "TLS read" );
	}
      default:
	throw std::string( "Unknown SSL error during read." );
      }
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      int tmp( gnutls_record_recv( m_tls_state->session, ptr, n ) );
      if( tmp==GNUTLS_E_INTERRUPTED || tmp==GNUTLS_E_AGAIN ) {
	return read( ptr, n );
      } else if( tmp < 0 ) {
	throw Exception::fderror( tmp, this->fd(), "GNUTLS recv" );
      }
      return tmp;
#endif
#endif
    }
    
      template < typename C, typename T >
    void basic_tlsstreambuf < C, T > ::really_close() {
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
      if( m_tls_state->ssl ) {
	SSL_shutdown( m_tls_state->ssl );
	SSL_free( m_tls_state->ssl );
      }
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      gnutls_bye( m_tls_state->session, GNUTLS_SHUT_WR );
#endif
#endif
      basic_fdstreambuf<C,T>::really_close();
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      gnutls_deinit( m_tls_state->session );
#endif
      delete m_tls_state;
      m_tls_state = 0;
    }
    
    template < typename C, typename T >
    bool basic_tlsstreambuf < C, T > ::starttls() {
      setup();
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
      m_tls_state->ssl = SSL_new( m_tls_state->ctx );
      SSL_set_fd( m_tls_state->ssl, this->fd() );
      int ret;
      if( m_server ) {
	ret=SSL_accept( m_tls_state->ssl );
      } else {
	ret=SSL_connect( m_tls_state->ssl );
      }
      long tmp( SSL_get_error( m_tls_state->ssl, ret ) );
      switch( tmp ) {
      case SSL_ERROR_NONE:
	return true;
      }
      return false;
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      gnutls_transport_set_ptr( m_tls_state->session, reinterpret_cast<gnutls_transport_ptr>(this->fd()) );
      int tmp( gnutls_handshake( m_tls_state->session ) );
      while( tmp!=0 ) {
	if( gnutls_error_is_fatal( tmp ) ) {
	  return false;
	}
	tmp = gnutls_handshake( m_tls_state->session );
      }
#endif
#endif
      return true;
    }
    
    template < typename C, typename T >
    inline unsigned long int basic_tlsstreambuf<C,T>::bits() const {
#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
      return SSL_get_cipher_bits( m_tls_state->ssl, NULL );
#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
      return gnutls_cipher_get_key_size( gnutls_cipher_get( m_tls_state->session ) );
#endif
#endif
    }
    
  }
}
