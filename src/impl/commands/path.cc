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

// Copyright 2004 Dave Cridland

#include <infotrope/server/command.hh>
#include <infotrope/datastore/path.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope::Data;
using namespace Infotrope;

namespace {
  
  class XPath : public Command {
  public:
    XPath( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() >= 3 ) {
	if( !m_toks->get(2).isAtom() ) {
	  throw std::runtime_error( "Need operation" );
	} else {
	  istring op( m_toks->get(2).toAtom().value() );
	  if( op=="USER" ) {
	    if( complete && m_toks->length() != 5 ) {
	      throw std::runtime_error( "XPATH USER <string: path> <string: user>" );
	    }
	    if( m_toks->length() >= 4 && !m_toks->get(3).isString() ) {
	      throw std::runtime_error( "XPATH USER <string: path> <string: user>" );
	    }
	    if( m_toks->length() == 5 && !m_toks->get(4).isString() ) {
	      throw std::runtime_error( "XPATH USER <string: path> <string: user>" );
	    }
	    if( m_toks->length() > 5 ) {
	      throw std::runtime_error( "XPATH USER <string: path> <string: user>" );
	    }
	  } else if( op=="CANONICAL" ) {
	    if( complete && m_toks->length() != 4 ) {
	      throw std::runtime_error( "XPATH CANONICAL <string: path>" );
	    }
	    if( m_toks->length() == 4 && !m_toks->get(3).isString() ) {
	      throw std::runtime_error( "XPATH CANONICAL <string: path>" );
	    }
	    if( m_toks->length() > 4 ) {
	      throw std::runtime_error( "XPATH CANONICAL <string: path>" );
	    }
	  } else {
	    throw std::runtime_error( "XPATH {USER|CANONICAL}" );
	  }
	}
      }
    }
    
    void main() {
      Utils::magic_ptr<Path> p;
      if( m_toks->get( 2 ).toAtom().value()=="USER" ) {
	p = new Path( m_toks->get( 3 ).toString().value(), m_toks->get( 4 ).toString().value() );
      } else if( m_toks->get( 2 ).toAtom().value()=="CANONICAL" ) {
	p = new Path( m_toks->get( 3 ).toString().value() );
      }
      {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "XPATH" );
	l->add( "CANONICAL" );
	l->add( p->canonicalize().asString() );
	m_worker.send( l );
      }
      {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "XPATH" );
	l->add( "SUPPLIED" );
	l->add( p->asString() );
	m_worker.send( l );
      }
      {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "OK" );
	l->add( std::string( "Path expansion complete" ) );
	m_worker.send( l, true );
      }
    }
    
  };
  
  Command::Register<XPath> r( "XPATH", INIT );
}
