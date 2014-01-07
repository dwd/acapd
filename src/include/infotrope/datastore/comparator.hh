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
#ifndef INFOTROPE_DATASTORE_COMPARATOR_HH
#define INFOTROPE_DATASTORE_COMPARATOR_HH

/*
  These should all be usable as Less<> replacements.
*/

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/server/token.hh>
#include <infotrope/datastore/attribute.hh>

namespace Infotrope {
  
  namespace Comparators {
    
    typedef Utils::magic_ptr<Data::Attribute> t_arg;
    typedef Utils::magic_ptr<Token::Token> t_tok;
    
    class Comparator {
    protected:
      bool m_positive;
      std::string const m_name;
    private:
      int m_magic_num;
    public:
      Comparator( std::string const & name, int magic_num ) : m_positive( false ), m_name( name ), m_magic_num( magic_num ) {}
      
      
      Comparator( bool p, std::string const & name, int magic_num ) : m_positive( p ), m_name( name ), m_magic_num( magic_num ) {}
      
      
      virtual ~Comparator() {}
      
      
      typedef enum {
	Lower,
	Equal,
	Higher
      } compare_result;
      compare_result order( t_arg const & a,
			    t_arg const & b, bool nil_different );
      bool equals( t_arg const & a,
		   t_arg const & b );
      bool equals( t_tok const & a,
		   t_tok const & b );
      virtual bool equals_internal( t_tok const & a,
				    t_tok const & b );
      bool compare( t_arg const & a,
		    t_arg const & b );
      bool comparestrict( t_arg const & a,
			  t_arg const & b );
      virtual bool comparestrict_internal( t_tok const & a,
					   t_tok const & b );
      bool prefix( t_arg const & a,
		   t_arg const & b );
      bool prefix( t_tok const & a,
		   t_tok const & b );
      virtual bool prefix_internal( t_tok const & a,
				    t_tok const & b );
      bool substring( t_arg const & a,
		      t_arg const & b );
      bool substring( t_tok const & a,
		      t_tok const & b );
      virtual bool substring_internal( t_tok const & a,
				       t_tok const & b );
      
      t_tok const & transform( t_arg const & a );
      virtual t_tok transform_internal( t_arg const & a ) = 0;
      static Utils::magic_ptr < Comparator > const & comparator( std::string const & );
    };
    
    class Octet : public Comparator {
    public:
      Octet( std::string const & n ) : Comparator( n, 0 ) {}
      
      
      Octet( bool p, std::string const & n ) : Comparator( p, n, 0 ) {}
      
      
      
      bool comparestrict_internal( t_tok const & a,
				   t_tok const & b );
      bool equals_internal( t_tok const & a,
			    t_tok const & b );
      bool prefix_internal( t_tok const & a,
			    t_tok const & b );
      bool substring_internal( t_tok const & a,
			       t_tok const & b );
      t_tok transform_internal( t_arg const & );
    };
    
    class AsciiNumeric : public Comparator {
    public:
      AsciiNumeric( std::string const & n ) : Comparator( n, 1 ) {}
      
      
      AsciiNumeric( bool p, std::string const & n ) : Comparator( p, n, 1 ) {}
      
      
      
      bool comparestrict_internal( t_tok const & a,
				   t_tok const & b );
      bool equals_internal( t_tok const & a,
			    t_tok const & b );
      t_tok transform_internal( t_arg const & );
    };
    
    class AsciiCasemap : public Comparator {
    public:
      AsciiCasemap( std::string const & n ) : Comparator( n, 2 ) {}
      
      
      AsciiCasemap( bool p, std::string const & n ) : Comparator( p, n, 2 ) {}
      //bool compare( t_arg const & a,
      //		 t_arg const & b );
      //bool comparestrict( t_arg const & a,
      //	      t_arg const & b );
      
      
      bool comparestrict_internal( t_tok const & a,
				   t_tok const & b );
      bool equals_internal( t_tok const & a,
			    t_tok const & b );
      bool prefix_internal( t_tok const & a,
			    t_tok const & b );
      bool substring_internal( t_tok const & a,
			       t_tok const & b );
      
      t_tok transform_internal( t_arg const & );
    };
    
  }
}

#endif
