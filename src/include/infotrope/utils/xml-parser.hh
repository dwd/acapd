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
#ifndef XML_PARSER
#define XML_PARSER

// An XML parser based on expat.
// Why expat? Because it was the first XML parser I happened to find.

#include <string>
#include <map>
#include <list>
#include <infotrope/utils/magic-ptr.hh>

namespace Infotrope {
  namespace XML {
    typedef std::map < std::string, std::string > t_attrs;
    typedef Infotrope::Utils::magic_ptr < t_attrs > t_attrs_p;
    
    class ParserInternals;
    // Cheshire Cat.
    
    class Tag {
    public:
      t_attrs_p attrs;
      std::string tag;
      std::string text;
      
      Tag( std::string const &, t_attrs_p const & );
      ~Tag();
    };
    
    class Parser {
    public:
      virtual ~Parser();
      void parse( std::string const &, bool final = false );
      typedef std::map < std::string, std::string > t_attrs;
      
    protected:
      Parser();
      
    private:
      static void st_start( void *, const char *, const char ** );
      static void st_end( void *, const char * );
      static void st_text( void *, const char *, int );
      
    protected:
      virtual void start( std::string const &, t_attrs_p const & );
      virtual void end( std::string const & );
      virtual void text( std::string const & );
      void addtext( std::string const & );
      
      typedef std::list < Tag > t_tags;
      t_tags m_tags;
      
      Tag & current_tag() {
	t_tags::iterator i( m_tags.end() );
	--i;
	return *i;
      }
      
    private:
      ParserInternals * m_int;
    };
  }
}

#endif
