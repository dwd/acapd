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
#include <infotrope/stream/tlsstreambuf.hh>
#ifdef INFOTROPE_QC
#include <infotrope/stream/tlsstreambuf.tcc>
#endif
#include <infotrope/stream/tlsstate.hh>
#include <map>

#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#include <infotrope/threading/rw-lock.hh>
#include <vector>
#include <unistd.h>
#endif

//#include <iostream>

using namespace Infotrope::Stream;
using namespace std;

#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H

#ifdef _REENTRANT
struct CRYPTO_dynlock_value : public Infotrope::Threading::RWMutex {
  CRYPTO_dynlock_value() : Infotrope::Threading::RWMutex() {
  }
};
#endif

namespace {
#ifdef _REENTRANT
  std::vector<Infotrope::Threading::Mutex> * tls_locks;
  static CRYPTO_dynlock_value *  dyn_create( const char * f, int l ) {
    //std::cout << "Creating dynlock for " << f << ":" << l << std::endl << std::flush;
    return new CRYPTO_dynlock_value;
  }
  static void dyn_lock( int mode, CRYPTO_dynlock_value * m, const char * f, int l ) {
    //std::cout << "Action on dynlock for " << f << ":" << l << std::endl << std::flush;
    if( mode&CRYPTO_LOCK ) {
      if( mode&CRYPTO_READ ) {
	m->acquire_read();
      } else {
	m->acquire_write();
      }
    } else {
      if( mode&CRYPTO_READ ) {
	m->release_read();
      } else {
	m->release_write();
      }
    }
  }
  static void dyn_destroy( CRYPTO_dynlock_value * m, const char * f, int l ) {
    //std::cout << "Destroying dynlock for " << f << ":" << l << std::endl << std::flush;
    delete m;
  }
  static void locking_function( int mode, int which, const char * f, int l ) {
    //std::cout << "Lock for " << which << " in " << f << ":" << l << std::endl << std::flush;
    if( mode&CRYPTO_LOCK ) {
      (*tls_locks)[which].acquire();
    } else {
      (*tls_locks)[which].release();
    }
  }
  //static unsigned long int id_function( void ) {
  //  return getpid();
  //}
  
#endif
  class tls {
  public:
    tls() {
      SSLeay_add_ssl_algorithms();
      SSL_load_error_strings();
#ifdef _REENTRANT
      CRYPTO_set_dynlock_create_callback( dyn_create );
      CRYPTO_set_dynlock_lock_callback( dyn_lock );
      CRYPTO_set_dynlock_destroy_callback( dyn_destroy );
      CRYPTO_set_locking_callback( locking_function );
      //CRYPTO_set_id_callback( id_function );
      tls_locks = new std::vector<Infotrope::Threading::Mutex>( CRYPTO_num_locks() );
#endif
    }
  };
  tls * tls_singleton(0);
#ifdef _REENTRANT
  Infotrope::Threading::Mutex tls_lock;
#endif
  std::map< bool, std::map< std::string, SSL_CTX * > > ctxmap;
}

tls_state::tls_state( bool server, std::string const & certfile ) : method( 0 ), ssl( 0 ), ctx( 0 ) {
#ifdef _REENTRANT
  Infotrope::Threading::Lock l__inst( tls_lock );
#endif
    
  if( !tls_singleton ) {
    tls_singleton = new tls;
  }
  
  if( !ctxmap[server][certfile] ) {
    if( server ) {
      method = SSLv23_server_method();
    } else {
      method = SSLv23_client_method();
    }
    if( !method ) {
      throw std::string( "No method!" );
    }
    //std::cout << "Creating context.\n" << std::flush;
    ctx = SSL_CTX_new( method );
    //std::cout << "Created " << ctx << "\n" << std::flush;
    if( !ctx ) {
      unsigned long err( ERR_get_error() );
      char buf[1024];
      ERR_error_string_n( err, buf, 1024 );
      throw std::string( "No context: " ) + buf;
    }
    if( certfile.length() ) {
      if( 1!=SSL_CTX_use_certificate_file( ctx, certfile.c_str(), SSL_FILETYPE_PEM ) ) {
	throw std::string( "Cert file bogus." );
      }
      if( 1!=SSL_CTX_use_PrivateKey_file( ctx, certfile.c_str(), SSL_FILETYPE_PEM ) ) {
	throw std::string( "Key file bogus" );
      }
    }
    ctxmap[server][certfile] = ctx;
  } else {
    ctx = ctxmap[server][certfile];
  }
}

#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H

namespace {
  
  class tls {
  public:
    tls() {
      gnutls_global_init();
      gnutls_dh_params_init( &dh_params );
      gnutls_datum x, y;
      gnutls_dh_params_generate( &x, &y, 1024 );
      gnutls_dh_params_set( dh_params, x, y, 1024 );
    }
    
    gnutls_dh_params dh_params;
    std::map< std::string, gnutls_certificate_credentials > cert_map;
  };
  
  tls * s_tls( 0 );
#ifdef _REENTRANT
  Infotrope::Threading::Mutex mutex;
#endif
}

tls_state::tls_state( bool server, std::string const & certfile ) {
#ifdef _REENTRANT
  Infotrope::Threading::Lock l__inst( mutex );
#endif
  {
    if( !s_tls ) {
      s_tls = new tls();
    }
  }
  gnutls_init( &session, server?GNUTLS_SERVER:GNUTLS_CLIENT );
  gnutls_set_default_priority( session );
  if( s_tls->cert_map.find( certfile )==s_tls->cert_map.end() ) {
    gnutls_certificate_allocate_credentials( &s_tls->cert_map[certfile] );
    gnutls_certificate_set_x509_key_file( s_tls->cert_map[certfile], certfile.c_str(), certfile.c_str(), GNUTLS_X509_FMT_PEM );
  }
  gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, s_tls->cert_map[certfile] );
  gnutls_dh_set_prime_bits( session, 1024 );
}
#else /* Nothing defined */
#error "No TLS implementation supported."
#endif
#endif

#ifdef INFOTROPE_QC
template class basic_tlsstreambuf<char>;
#endif

