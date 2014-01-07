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
#include <infotrope/datastore/datastore.hh>
#include <infotrope/server/master.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Data;
using namespace Infotrope;

namespace {
  
  class Compress : public Command {
  public:
    Compress( Worker & worker ) : Command( true, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 3 ) {
	throw std::runtime_error( "Too many arguments." );
      }
    }
    
    void main() {
      Master::master()->log( 4, "Switching to Deflate." );
      magic_ptr<Token::List> l( new Token::List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "OK" );
      l->add( std::string( "Now doing Deflate goodness." ) );
      m_worker.send( l, true );
      if( !m_worker.startdeflate() ) {
	m_worker.post_message( Worker::Shutdown );
	throw std::runtime_error( "Damn. Bet this has arsed up completely." );
      }
    }
    
  };
  
  Command::Register<Compress> r( "Compress", PREAUTH );
}
