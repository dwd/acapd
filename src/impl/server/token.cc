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
#include <infotrope/server/token.hh>
#include <infotrope/utils/cast-exception.hh>
#include <infotrope/utils/istring.hh>
#include <sstream>

using namespace Infotrope::Token;
using namespace std;

Token::Token( bool nil ) : m_nil( nil ) {
}

string Token::asString() const {
  if ( isNil() ) {
    return "NIL";
  } else {
    return internal_asString();
  }
}

Token::~Token() {
}

List::List() : Token( false ) {
}

List::List( bool set ) : Token( set ) {
}

void List::add( Infotrope::Utils::magic_ptr < Token > const & t ) {
  assertNotNil( "Cannot add value for NIL List" );
  if( m_list.size()>0 ) {
    if( m_list.back()->isLiteralMarker() ) {
      m_list.pop_back();
    }
  }
  m_list.push_back( t );
}

string List::internal_asString() const {
  string text;
  for ( t_list::const_iterator i( m_list.begin() ); i != m_list.end(); ++i ) {
    if ( i != m_list.begin() ) {
      if ( i != m_list.end() ) {
        text += " ";
      }
    }
    text += ( *i ) ->asString();
  }
  return text;
}

AtomBase::AtomBase() : Token( true ) {
}

AtomBase::AtomBase( std::string const & s ) : Token( false ), m_text( s ) {
}

Atom::Atom() : AtomBase() {
}

Atom::Atom( const std::string & s ) : AtomBase( s ), m_istring( Infotrope::Utils::istring_convert( m_text ) ) {
}

string AtomBase::internal_asString() const {
  return m_text;
}

Infotrope::Utils::istring const & Atom::value() const {
  assertNotNil( "Cannot get value for NIL Atom" );
  return m_istring;
}

/*long int Atom::raw_asInt() const {
  stringstream ss( m_text.c_str() );
  long int i;
  ss >> i;
  return i;
  }*/

namespace {
  string itos( int i ) {
    stringstream ss;
    ss << i;
    string s( ss.str() );
    return s;
  }
}

// itos stuff moved into Integer.
//Atom::Atom( long int i ) : Token( false ), m_text( itos( i ) ) {
//}

String::String() : AtomBase() {
}

String::String( const std::string & s ) : AtomBase( s ) {
}

string String::internal_asString() const {
  if ( ( m_text.length() > 80 )
       || ( m_text.find_first_of( "\\\r\n\"" ) != string::npos ) ) {
    return literal_asString();
  } else {
    return quoted_asString();
  }
}

string const & String::value() const {
  assertNotNil( "Cannot get value for NIL String" );
  return m_text;
}

string String::literal_asString( bool ns ) const {
  ostringstream ss;
  ss << "{" << m_text.length();
  if( ns ) {
    ss << "+";
  }
  ss << "}\r\n" << m_text;
  return ss.str();
}

namespace {
  const char escapeThese[] = "\\\"";
  const char unsafe[] = "\r\n\0";
}

string String::quoted_asString() const {
  string::size_type l( 0 );
  string tmp( "\"" );
  for ( string::size_type i( m_text.find_first_of( escapeThese ) ); i != string::npos;
        i = m_text.find_first_of( escapeThese, i + 1 ) ) {
    tmp += m_text.substr( l, i - l );
    tmp += '\\';
    l = i;
  }
  if ( l < m_text.length() ) {
    tmp += m_text.substr( l );
  }
  tmp += '\"';
  return tmp;
}

LiteralString::LiteralString() : String() {
}

LiteralString::LiteralString( const std::string & s ) : String( s ) {
}

string LiteralString::internal_asString() const {
  return String::literal_asString();
}

LiteralMarker::LiteralMarker( unsigned int l, bool ns ) : Token( false ), m_length(l), m_nonsync(ns) {
}

std::string LiteralMarker::internal_asString() const {
  std::ostringstream ss;
  ss << '{' << m_length;
  if( m_nonsync ) {
    ss << '+';
  }
  ss << '}';
  return ss.str();
}

NSLiteralString::NSLiteralString() : String() {
}

NSLiteralString::NSLiteralString( const std::string & s ) : String( s ) {
}

string NSLiteralString::internal_asString() const {
  return String::literal_asString(true);
}

QuotedString::QuotedString() : String() {
}

QuotedString::QuotedString( const std::string & s ) : String( s ) {
}

string QuotedString::internal_asString() const {
  return String::quoted_asString();
}

PList::PList() : List() {
}

PList::PList( bool nil ) : List( nil ) {
}

string PList::internal_asString() const {
  return "(" + List::internal_asString() + ")";
}

Integer::Integer() : AtomBase(), m_value( 0 ) {
}

Integer::Integer( long int i ) : AtomBase( itos( i ) ), m_value( i ) {
}

long int Integer::value() const {
  assertNotNil( "Cannot get value for NIL Integer" );
  return m_value;
}

Token & Token::toToken() {
  return *this;
}
Token const & Token::toToken() const {
  return *this;
}
bool Token::isToken() const {
  return true;
}

/*
  I appreciate that this is horrible.
*/

#define TOKEN_CAST_DEF( type ) \
  type & Token::to##type() {                                            \
    if( isNil() ) {                                                     \
      throw Utils::Exceptions::cast( "NIL", typeid( type ).name(), this->asString() ); \
    }                                                                   \
    try {                                                               \
      return dynamic_cast<type &>( *this );                             \
    } catch( std::bad_cast & e ) {                                      \
      throw Utils::Exceptions::cast( typeid( *this ).name(), typeid( type ).name(), this->asString() ); \
    }                                                                   \
  }                                                                     \
                                                                        \
  type const & Token::to##type() const {                                \
    if( isNil() ) {                                                     \
      throw Utils::Exceptions::cast( "NIL", typeid( type ).name(), this->asString() ); \
    }                                                                   \
    try {                                                               \
      return dynamic_cast<type const &>( *this );                       \
    } catch( std::bad_cast & e ) {                                      \
      throw Utils::Exceptions::cast( typeid( *this ).name(), typeid( type const ).name(), this->asString() ); \
    }                                                                   \
  }                                                                     \
                                                                        \
  bool Token::is##type() const {                                        \
    if( isNil() ) {                                                     \
      return false;                                                     \
    }                                                                   \
    try {                                                               \
      type const * x( dynamic_cast<type const *>( this ) );             \
      return x;                                                         \
    } catch( std::bad_cast & e ) {                                      \
      return false;                                                     \
    }                                                                   \
  }

TOKEN_CAST_DEF( List );
TOKEN_CAST_DEF( PList );
TOKEN_CAST_DEF( Atom );
TOKEN_CAST_DEF( AtomBase );
TOKEN_CAST_DEF( String );
TOKEN_CAST_DEF( LiteralString );
TOKEN_CAST_DEF( QuotedString );
TOKEN_CAST_DEF( Integer );
TOKEN_CAST_DEF( LiteralMarker );
