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

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope;

namespace {
  
  class Sleep : public Command {
  public:
    Sleep( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 3 ) {
	throw std::runtime_error( "Too many arguments." );
      }
      if( m_toks->length() == 3 && !m_toks->get(2).isInteger() ) {
	throw std::runtime_error( "Incorrect argument - need Integer." );
      }
    }
    
    void main() {
      sleep( m_toks->get(2).toInteger().value() );
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "OK" );
      l->add( std::string( "*yawn*" ) );
      m_worker.send( l, true );
    }
    
  };
  
  Command::Register<Sleep> r( "SLEEP", INIT );
}
