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
  
  class GetQuota : public Command {
  public:
    GetQuota( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 2 ) {
	if( m_toks->get( 2 ).isString() ) {
	  Path p( m_toks->get( 2 ).toString().value() );
	} else if( !m_toks->get( 2 ).isLiteralMarker() ) {
	  throw std::runtime_error( "Dataset path required must be string." );
	}
      }
      if( m_toks->length() > 3 ) {
	throw std::runtime_error( "Too many arguments." );
      }
    }
    
    void main() {
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "OK" );
      l->add( std::string( "Unimplemented, sorry." ) );
      m_worker.send( l, true );
    }
    
  };
  
  Command::Register<GetQuota> r( "GETQUOTA", AUTH );
}
