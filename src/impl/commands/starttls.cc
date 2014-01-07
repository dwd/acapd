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
  
  class StartTls : public Command {
  public:
    StartTls( Worker & worker ) : Command( true, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 2 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 2 ) {
	throw std::runtime_error( "Too many arguments." );
      }
    }
    
    void main() {
      std::string cert;
      Master::master()->log( 4, "Switching to TLS." );
      const Datastore & ds( Datastore::datastore_read() );
      Threading::ReadLock l__inst( ds.lock() );
      Master::master()->log( 4, "Got locks." );
      magic_ptr<Dataset> dset( ds.dataset( Path( "/vendor.infotrope.acapd/site/" ) ) );
      magic_ptr<Entry> e( dset->fetch2( "tls_certificate" ) );
      Master::master()->log( 4, "Fetched entry." );
      if( !e || !e->exists( "vendor.infotrope.acapd.value" ) ) {
	throw std::runtime_error( "Not configured." );
      }
      cert = e->attr( "vendor.infotrope.acapd.value" )->valuestr();
      magic_ptr<Token::List> l( new Token::List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "OK" );
      l->add( std::string( "Ready for full-on TLS goodness." ) );
      m_worker.send( l, true );
      if( !m_worker.starttls( cert ) ) {
	m_worker.post_message( Worker::Shutdown );
	throw std::runtime_error( "Damn. Bet this has arsed up completely." );
      }
      m_worker.banner();
    }
    
  };
  
  Command::Register<StartTls> r( "StartTls", PREAUTH );
}
