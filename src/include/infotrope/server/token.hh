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
#ifndef INFOTROPE_SERVER_TOKEN_HH
#define INFOTROPE_SERVER_TOKEN_HH

#include <string>
#include <vector>
#include <infotrope/utils/istring.hh>

//#include "imap.hh"
#include <infotrope/utils/magic-ptr.hh>

//#include <infotrope/server/master.hh>

#include <stdexcept>

namespace Infotrope {
  namespace Token {
    class Token;   // Generic base token.
    class List;    // A list of tokens - for example a command or response.
    class PList;   // A Parenthesized List - used for sublists within a command.
    class Atom;    // An atom, case insensitive.
    class AtomBase;   // A thing which is scalar.
    class QuotedString;   // "something like \"this\""
    class String;   // Might be quoted or literal, but either way it's a case sensitive string.
    class Integer;   // A long unsigned int transmitted as a bare string.
    class LiteralString;   // A literal.
    class LiteralMarker; // A literal marker: {123}
    
    class Token {
    protected:
      Token( bool nil );
    public:
      virtual ~Token();
      
    public:
      std::string asString() const;   // To transmittable string.
      
      // Dynamic casting functions.
      Token & toToken();
      List & toList();
      PList & toPList();
      Atom & toAtom();
      AtomBase & toAtomBase();
      String & toString();
      QuotedString & toQuotedString();
      LiteralString & toLiteralString();
      Integer & toInteger();
      LiteralMarker & toLiteralMarker();
      
      Token const & toToken() const;
      List const & toList() const;
      PList const & toPList() const;
      AtomBase const & toAtomBase() const;
      Atom const & toAtom() const;
      String const & toString() const;
      QuotedString const & toQuotedString() const;
      LiteralString const & toLiteralString() const;
      Integer const & toInteger() const;
      LiteralMarker const & toLiteralMarker() const;
      
      // Querying functions, which actually just do the dynamic cast
      // and see if it works.
      bool isToken() const;
      bool isList() const;
      bool isPList() const;
      bool isAtom() const;
      bool isAtomBase() const;
      bool isString() const;
      bool isQuotedString() const;
      bool isLiteralString() const;
      bool isInteger() const;
      bool isLiteralMarker() const;
      bool isNil() const {
	return !this||m_nil; // Hack to allow a NULL pointer to behave like a NIL.
      }
      void assertNotNil( const char * arg ) const {
	/*Infotrope::Server::Master::master()->log( 1, "Asserting NOT NIL." );
	  if( !this ) {
	  Infotrope::Server::Master::master()->log( 1, "NULL this pointer." );
	  }
	  if( this && m_nil ) {
	  Infotrope::Server::Master::master()->log( 1, "Formal NIL." );
	  }
	*/
	if( isNil() ) {
	  //Infotrope::Server::Master::master()->log( 1, "But we're NIL!" );
	  throw std::runtime_error( std::string( "Assertion of not NIL failed for " )+arg );
	}
	//Infotrope::Server::Master::master()->log( 1, "And we're now here, so we must be okay." );
      }
      
    protected:
      virtual std::string internal_asString() const = 0;
      
      bool m_nil;
    };
    
    class AtomBase : public Token {
    protected:
      AtomBase();
      AtomBase( const std::string & );
      
      std::string internal_asString() const;
      std::string m_text;
    };
    
    class Atom : public AtomBase {
    public:
      Atom();
      Atom( const std::string & );
      //Atom( long int );
      
      const Utils::istring & value() const;
      
    protected:
      Utils::istring m_istring;
    };
    
    class String : public AtomBase {
    public:
      String();
      String( const std::string & );
      
      const std::string & value() const;
      
    protected:
      std::string internal_asString() const;
      std::string literal_asString( bool ns=false ) const;
      std::string quoted_asString() const;
    };
    
    class List : public Token {
    public:
      List();
      List( bool );
      
      typedef std::vector < Infotrope::Utils::magic_ptr < Token > > t_list;
      
      /*const t_list & list() const {
	return m_list;
	}*/
      Token const & get( int i ) const {
	assertNotNil( "get element of list." );
	if ( i >= length() ) throw std::runtime_error( "Unexpected end of list in " + asString() );
	return *m_list[ i ];
      }
      Utils::magic_ptr < Token > const & ptr( int i ) const {
	//Infotrope::Server::Master::master()->log( 1, "Getting element." );
	assertNotNil( "Tried to get pointer of element of NIL list." );
	//Infotrope::Server::Master::master()->log( 1, "Not NIL" );
	if ( i >= length() ) throw std::runtime_error( "Unexpected end of list in " + asString() );
	//Infotrope::Server::Master::master()->log( 1, "Within length." );
	return m_list[ i ];
      }
      
      int length() const {
	//Infotrope::Server::Master::master()->log( 1, "Getting size of list." );
	assertNotNil( "Tried to get size of NIL list" );
	//Infotrope::Server::Master::master()->log( 1, "We are apparently not NIL." );
	return m_list.size();
      }
      void add( Utils::magic_ptr < Token > const & );
      void add( Token * t ) {
	add( Infotrope::Utils::magic_ptr<Token>( t ) );
      }
      void add( const char * t ) {
	add( Infotrope::Utils::magic_ptr<Token>( new Atom( t ) ) );
      }
      void add( std::string const & t ) {
	add( Infotrope::Utils::magic_ptr<Token>( new String( t ) ) );
      }
      t_list const & value() const {
	assertNotNil( "Cannot get value of NIL List" );
	return m_list;
      }
    protected:
      std::string internal_asString() const;
      
    private:
      t_list m_list;
    };
    
    class PList : public List {
    public:
      PList();
      PList( bool );
    protected:
      std::string internal_asString() const;
    };
    
    class LiteralString : public String {
    public:
      LiteralString();
      LiteralString( const std::string & );
      
    protected:
      std::string internal_asString() const;
    };
    
    class LiteralMarker : public Token {
    public:
      LiteralMarker( unsigned int l, bool ns );
      
      long unsigned int length() const {
	return m_length;
      }
      bool nonSync() const {
	return m_nonsync;
      }
      
    protected:
      std::string internal_asString() const;
      long unsigned int m_length;
      bool m_nonsync;
    };
    
    class NSLiteralString : public String {
    public:
      NSLiteralString();
      NSLiteralString( const std::string & );
      
    protected:
      std::string internal_asString() const;
    };
    
    class QuotedString : public String {
    public:
      QuotedString();
      QuotedString( const std::string & );
      
    protected:
      std::string internal_asString() const;
    };
    
    class Integer : public AtomBase {
    public:
      Integer();
      Integer( long int );
      
      long int value() const;
    private:
      long int m_value;
    };
    
    inline std::ostream & operator<<( std::ostream & o, Token & t ) {
      o << t.asString();
      return o;
    }
  }
}
  
#endif
