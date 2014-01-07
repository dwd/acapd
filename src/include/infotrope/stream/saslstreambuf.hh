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
#ifndef SASL_STREAMBUF_HH
#define SASL_STREAMBUF_HH

#include <infotrope/stream/fdstreambuf.hh>

#include <infotrope/config.h>

#ifndef INFOTROPE_HAVE_SASL_SASL_H
#error "I should not be being compiled."
#endif

extern "C" {
  typedef struct sasl_conn sasl_conn_t;
}

namespace Infotrope {
  namespace Stream {
    
    template < typename C, typename T = std::char_traits < C > >
    class basic_saslstreambuf : public basic_fdstreambuf < C, T > {
    public:
      basic_saslstreambuf( struct sasl_conn *, basic_fdstreambuf < C, T > * );
      ~basic_saslstreambuf();
      long unsigned bits() const;
      
    protected:
      virtual int write( char * ptr, int n );
      virtual int read( char * ptr, int n );
      
    private:
      struct sasl_conn * m_sasl_conn;
      unsigned m_maxoutbuf;
      basic_fdstreambuf<C,T> * m_slave;
    };

#ifdef INFOTROPE_QC    
    extern template class basic_saslstreambuf<char>;
#endif
    
  }
}

#ifndef INFOTROPE_QC
#include "saslstreambuf.tcc"
#endif

#endif
