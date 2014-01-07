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
#ifndef TLS_STATE_HH
#define TLS_STATE_HH

#include <infotrope/config.h>
#include <string>

#ifdef INFOTROPE_HAVE_OPENSSL_CRYPTO_H
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Infotrope {
  namespace Stream {
    
    class tls_state {
    public:
      tls_state( bool, std::string const & );
      
      SSL_METHOD const * method;
      SSL * ssl;
      SSL_CTX * ctx;
    };
  }
}

#else
#ifdef INFOTROPE_HAVE_GNUTLS_GNUTLS_H
#include <gnutls/gnutls.h>

namespace Infotrope {
  namespace Stream {
    
    class tls_state {
    public:
      tls_state( bool, std::string const & );
      
      gnutls_session session;
    };
  }
}

#else
#error "I shouldn't be being compiled - no TLS support."
#endif
#endif

#endif
