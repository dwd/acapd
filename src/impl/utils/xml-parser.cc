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
#include <infotrope/utils/xml-parser.hh>
#include <expat.h>
#include <sstream>

namespace Infotrope {
  namespace XML {
    class ParserInternals {
    public:
      XML_Parser parser;
    };
  }
}

using namespace Infotrope::XML;

Parser::~Parser() {
  XML_ParserFree( m_int->parser );
  delete m_int;
}

void Parser::parse( std::string const & s, bool final ) {
  if ( !XML_Parse( m_int->parser, s.c_str(), s.length(), final ) ) {
    std::ostringstream ss;
    ss << "XML Parse Error at line " << XML_GetCurrentLineNumber( m_int->parser ) << ":\n" << XML_ErrorString( XML_GetErrorCode( m_int->parser ) ) << "\n" << s << "\n";
    throw std::runtime_error( ss.str() );
  }
}

Parser::Parser() : m_int( new ParserInternals ) {
  m_int->parser = XML_ParserCreate( "UTF-8" );
  XML_SetUserData( m_int->parser, this );
  XML_SetElementHandler( m_int->parser, st_start, st_end );
  XML_SetCharacterDataHandler( m_int->parser, st_text );
}

void Parser::st_start( void * self, const char * tag, const char ** attrs ) {
  Parser * me( reinterpret_cast < Parser * > ( self ) );
  t_attrs_p a( new t_attrs );
  for ( int i( 0 ); attrs[ i ]; i += 2 ) {
    ( *a ) [ attrs[ i ] ] = attrs[ i + 1 ];
  }
  me->start( tag, a );
}

void Parser::st_end( void * self, const char * tag ) {
  Parser * me( reinterpret_cast < Parser * > ( self ) );
  if ( !me->current_tag().text.empty() ) {
    me->text( me->current_tag().text );
  }
  me->end( tag );
}

void Parser::st_text( void * self, const char * text, int len ) {
  Parser * me( reinterpret_cast < Parser * > ( self ) );
  me->addtext( std::string( text, text + len ) );
}

void Parser::addtext( std::string const & atext ) {
  current_tag().text += atext;
}

void Parser::start( std::string const & tag, t_attrs_p const & attrs ) {
  m_tags.push_back( Tag( tag, attrs ) );
}

void Parser::end( std::string const & tag ) {
  t_tags::iterator i( m_tags.end() );
  m_tags.erase( --i );
}

void Parser::text( std::string const & txt ) {
}

Tag::Tag( std::string const & s, t_attrs_p const & a )
  : attrs( a ), tag( s ), text() {
}

Tag::~Tag() {
}
