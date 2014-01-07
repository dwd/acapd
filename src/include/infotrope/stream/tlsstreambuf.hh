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
#ifndef INFOTROPE_STREAM_TLSSTREAMBUF_HH
#define INFOTROPE_STREAM_TLSSTREAMBUF_HH

#include <infotrope/stream/fdstreambuf.hh>

namespace Infotrope {
  namespace Stream {
    class tls_state;
    
    template < typename C, typename T = std::char_traits < C > >
    class basic_tlsstreambuf : public basic_fdstreambuf < C, T > {
    public:
      basic_tlsstreambuf( bool /* server */, basic_fdstreambuf < C, T > * );
      basic_tlsstreambuf( bool /* server */, int );
      basic_tlsstreambuf( bool /* server */, basic_fdstreambuf < C, T > *, std::string const & );
      basic_tlsstreambuf( bool /* server */, int, std::string const & );
      
      bool starttls();
      unsigned long int bits() const;
      
    protected:
      virtual void really_close();
      virtual int write( char * ptr, int n );
      virtual int read( char * ptr, int n );
      virtual void setup();
      
    private:
      tls_state * m_tls_state;
      bool m_server;
      std::string m_cert;
    };

#ifdef INFOTROPE_QC    
    extern template class basic_tlsstreambuf<char>;
#endif
    
  }
}

#ifndef INFOTROPE_QC
#include "tlsstreambuf.tcc"
#endif

#endif
