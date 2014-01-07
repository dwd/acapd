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
#ifndef FD_STREAMBUF_HH
#define FD_STREAMBUF_HH

#include <streambuf> 
#include <stdexcept>

namespace  Infotrope {
  
  namespace Stream {
    
    namespace Exception {
      
      class fderror : public std::runtime_error {
      public:
	fderror( int /* errno */, int /* fd */, std::string const & /* context info */ );
	const char * what() const throw();
	
      private:
	int m_errno;
	int m_fd;
      };
      
    }
    
    template < typename C, typename T = std::char_traits < C > >
    class basic_fdstreambuf : public std::basic_streambuf < C, T > {
    public:
      basic_fdstreambuf();
      basic_fdstreambuf( int );
      virtual ~basic_fdstreambuf();
      
      typedef T traits_type;
      typedef typename T::int_type int_type;
      typedef typename T::char_type char_type;
      
      void attach( int );
      
      void blocking( bool b ) {
	m_block = b;
      }
      bool blocking() const {
	return m_block;
      }
      
      void autoclose( bool b ) {
	m_autoclose = b;
      }
      bool autoclose() const {
	return m_autoclose;
      }
      
      int fd() {
	return m_fd;
      }
      
      void close();
      void maybe_close();
      virtual long unsigned bits() const;
    protected:
      virtual int_type underflow();   // Will block unless have_data().
      virtual int_type overflow( int_type = traits_type::eof() );   // Will not block.
    public:
      virtual int sync();   // Will block.
      virtual int read( char * ptr, int n );
      virtual int write( char * ptr, int n );
    protected:
      virtual void really_close();
      virtual void setup();
      
    protected:
      int m_fd;
      bool m_block;
      bool m_autoclose;
    private:
      char_type * m_buffer;
    };

    typedef basic_fdstreambuf<char> fdstreambuf;
#ifdef INFOTROPE_QC
    extern template class basic_fdstreambuf<char>;
#endif
  }
}

#ifndef INFOTROPE_QC
#include "fdstreambuf.tcc"
#endif

#endif
