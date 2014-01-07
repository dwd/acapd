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
#include <infotrope/datastore/modtime.hh>
#include <unistd.h>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope::Data;
using namespace Infotrope;

namespace {
  
  class Session : public Command {
  public:
    Session( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 2 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 3 ) {
	throw std::runtime_error( "Too many arguments." );
      }
    }
    
    void main() {
      // Really, I should:
      // 1) Check if we already have a session identifier. If so, then we should emit a NO.
      // 2) Technically optionally, see if the session identifier suggested exists, and attach it if needs be.
      // Latter is optional because no session is entirely persistent - persistence is up to the server entirely. So not persisting any session is entirely valid, but useless, behaviour.
      // 3) Optionally, but nicely, update all persistent contexts in the session.
      {
	if( m_toks->length()==3 ) {
	  m_worker.set_session( m_toks->get( 2 ).toString().value() );
	} else {
	  m_worker.set_session();
	}
      }
      {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "OK" );
	l->add( std::string( "Sessions are not yet actually persistent, sorry. This is a placeholder command." ) );
	m_worker.send( l, true );
      }
    }
    
  };
  
  Command::Register<Session> r1( "XSESSION", AUTH );
  Command::Register<Session> r2( "SESSION", AUTH );
}
