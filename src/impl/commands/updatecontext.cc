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
#include <infotrope/server/context.hh>
#include <infotrope/server/exceptions.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope;

namespace {
  
  class UpdateContext : public Command {
  public:
    UpdateContext( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      for( int i( 2 ); i<m_toks->length(); ++i ) {
	if( !m_toks->get( i ).isString() && !m_toks->get( i ).isLiteralMarker() ) {
	  throw std::runtime_error( "Context name expected." );
	}
      }
    }
    
    void main() {
      for( int i( 2 ); i < m_toks->length(); ++i ) {
	if( m_worker.context_exists( m_toks->get( i ).toString().value() ) ) {
	  magic_ptr<Context> c( m_worker.context( m_toks->get( i ).toString().value() ) );
	  if( c->isNotify() ) {
	    c->updatecontext();
	  } else {
	    throw Server::Exceptions::no( c->name() + " : Not a NOTIFY context." );
	  }
	} else {
	  throw Server::Exceptions::no( m_toks->get( i ).toString().value() + " : No such context." );
	}
      }
      {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "OK" );
	l->add( std::string( "This probably didn't do anything. Very good for sending it, anyway." ) );
	m_worker.send( l, true );
      }
    }
    
  };
  
  Command::Register<UpdateContext> r( "UPDATECONTEXT", AUTH );
}
