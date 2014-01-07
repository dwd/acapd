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
#ifndef TCP_STREAM_HH
#define TCP_STREAM_HH

#include <istream> 
#include <ostream> 
#include <string>
//#include <infotrope/config.h>

//#ifdef INFOTROPE_HAVE_TLS
#include <infotrope/stream/tlsstreambuf.hh>
//#endif
//#ifdef INFOTROPE_HAVE_SASL_SASL_H
#include <infotrope/stream/saslstreambuf.hh>
extern "C" {
  typedef struct sasl_conn sasl_conn_t;
}
//#endif
//#ifdef INFOTROPE_HAVE_ZLIB_H
#include <infotrope/stream/deflatestreambuf.hh>
//#endif

namespace Infotrope {
  namespace Stream {
    template < typename C, typename T = std::char_traits < C > >
    class basic_tcpstream : public std::basic_iostream < C, T > {
    public:
      basic_tcpstream();   // Connect follows.
      basic_tcpstream( int fd );   // Autoattach.
      ~basic_tcpstream();
      
      void attach( int );
      void autoclose( bool );
      bool autoclose() const;
      
      bool starttls();   // Used for STARTTLS commands.
      bool starttls( bool, std::string const & );
      unsigned long int bits() const; // SSF for or from SASL.
      
      void connect( std::string const & /* host */, unsigned short int /* port */ );
      bool connected();
      int fd() const;
#ifdef INFOTROPE_HAVE_SASL_SASL_H
      bool startsasl( sasl_conn_t * ); // SASL security layer support. Made TLS-like.
#else
      bool startsasl( void * );
#endif
      bool startdeflate();
      
      
    private:
#ifdef INFOTROPE_HAVE_TLS
      bool starttls( basic_tlsstreambuf < C, T > * );
#else
      bool starttls( void  * );
#endif
      unsigned long int m_bits;
    };
    
    typedef basic_tcpstream<char> tcpstream;
#ifdef INFOTROPE_QC
    extern template class basic_tcpstream<char>;
#endif
  }
}
#ifndef INFOTROPE_QC
#include "tcpstream.tcc"
#endif

#endif
