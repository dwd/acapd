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
#include <infotrope/datastore/data.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/attribute.hh>
#include <infotrope/datastore/value.hh>
#include <infotrope/server/master.hh>
#include <stdexcept>

using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace Infotrope::Datastore;
using namespace Infotrope::Server;
using namespace Infotrope;

namespace {
  
  class Fetch : public Command {
  public:
    Fetch( Worker & worker ) : Command( false, worker ) {
    }
    
    void internal_parse( bool complete ) {
      if( complete && m_toks->length() != 3 ) {
	throw std::runtime_error( "Too few arguments." );
      }
      if( m_toks->length() > 3 ) {
	throw std::runtime_error( "Too many arguments" );
      }
      if( m_toks->length()==3 ) {
	if( !m_toks->get(2).isString() ) {
	  throw std::runtime_error( "Need a path as a string." );
	} else {
	  Path p( m_toks->get(2).toString().value() );
	  if( p.isRemote() ) {
	    throw std::runtime_error( "Need a local path, sorry." );
	  }
	}
      }
    }
    
    void main() {
      Master::master()->log( 1, "FETCH executing." );
      {
	Transaction trans;
	Path p( m_toks->get(2).toString().value() );
	Master::master()->log( 1, "Got path "+p.asString() );
	magic_ptr<Dataset> d( Data::data()->dataset( p.parent() ) );
	Master::master()->log( 1, "Got dataset object." );
	magic_ptr<EntryOverlay> e( d->fetch_overlay( p.mybit(), true ) );
	Master::master()->log( 1, "Got data objects" );
	if( !e.ptr() ) {
	  Master::master()->log( 1, "Entry does not exist, will throw." );
	  throw std::runtime_error( "Entry does not exist." );
	}
	magic_ptr<List> l( new List );
	l->add( m_toks->ptr( 0 ) );
	l->add( "FETCH" );
	l->add( p.asString() );
	Master::master()->log( 2, "About to iterate." );
	for( Entry::const_iterator i( e->begin() ); i!=e->end(); ++i ) {
	  PList * pl = new PList();
	  pl->add( (*i).first.asString() );
	  Master::master()->log( 2, "Hitting attribute." );
	  Master::master()->log( 2, "Attribute is "+(*i).second->attribute().asString() );
	  Master::master()->log( 2, "Okay, adding to list." );
	  pl->add( (*i).second->attribute().asString() );
	  Master::master()->log( 2, "Hitting value." );
	  PList * pl2 = new PList();
	  pl2->add( (*i).second->value()->asToken() );
	  pl->add( pl2 );
	  l->add( pl );
	}
	Master::master()->log( 2, "FETCH Iteration complete." );
	m_worker.send( l );
	trans.commit();
      }
      magic_ptr<List> l( new List );
      l->add( m_toks->ptr( 0 ) );
      l->add( "OK" );
      l->add( std::string( "Path expansion complete" ) );
      m_worker.send( l, true );
    }
    
  };
  
  Command::Register<Fetch> r( "FETCH", INIT );
}
