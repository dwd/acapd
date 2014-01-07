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
#include <infotrope/datastore/datastore.hh>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope::Data;
using namespace Infotrope;

namespace {
  
  class DeletedSince : public Command {
  public:
    DeletedSince( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() < 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->get( 2 ).isString() ) {
	Path p( m_toks->get( 2 ).toString().value() );
      } else {
	throw std::runtime_error( "Need a path" );
      }
      if( m_toks->get( 3 ).isString() ) {
	Modtime m( m_toks->get( 3 ).toString().value() );
      } else {
	throw std::runtime_error( "Need a modtime" );
      }
      if( m_toks->length() > 4 ) {
	throw std::runtime_error( "Too many arguments." );
      }
    }
    
    void main() {
      Infotrope::Threading::ReadLock l__inst( Datastore::datastore_read().lock() );
      Path dsp( m_toks->get( 2 ).toString().value(), m_worker.login() );
      Utils::magic_ptr<Dataset> dset( Datastore::datastore_read().dataset( dsp ) );
      {
	magic_ptr<List> l( new List );
	l->add( "*" );
	l->add( "OK" );
	l->add( "Dataset " + dsp.asString() + " deletion modtime of " + dset->last_deletion().asString() );
	m_worker.send( l );
      }
      if( dset->last_deletion() > Modtime( m_toks->get( 3 ).toString().value() ) ) {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "NO" );
	magic_ptr<Infotrope::Token::Token> pl( new PList );
	pl->toList().add( "TOOOLD" );
	l->add( pl );
	l->add( std::string( "There have been deletions since then." ) );
	m_worker.send( l, true );
      } else {
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "OK" );
	l->add( std::string( "No deletions since then." ) );
	m_worker.send( l, true );
      }
    }
    
  };
  
  Command::Register<DeletedSince> r( "DELETEDSINCE", AUTH );
}
