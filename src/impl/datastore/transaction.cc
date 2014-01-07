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
 *            acap-transaction.cc
 *
 *  Thu Feb 13 12:47:09 2003
 *  Copyright  2003  Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/
#include <infotrope/datastore/transaction.hh>
#include <infotrope/threading/rw-lock.hh>
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/dataset.hh>
// #include <infotrope/server/context.hh>
#include <infotrope/datastore/xml-dump.hh>
#include <infotrope/utils/magic-ptr.hh>

#include <infotrope/server/master.hh>

#include <list>
#include <set>

using namespace Infotrope::Data;
using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Notify;
using namespace std;

namespace {
  class TransactionInstance {
  public:
    TransactionInstance()
      : m_main(), m_check(), m_set(), m_current( 0 ) {
    }
    
    Infotrope::Threading::Mutex & lock() {
      return m_main;
    }
    
    bool mine() {
      Infotrope::Threading::Lock l__inst( m_check );
      if ( !m_current ) {
	return false;
      }
      return pthread_equal( m_who, pthread_self() );
    }
    void set( Transaction * t ) {
      Infotrope::Threading::Lock l__inst( m_check );
      m_who = pthread_self();
      m_current = t;
      m_modtime = Modtime::modtime();
      m_poke_modtime = m_modtime;
    }
    Transaction & trans() {
      Infotrope::Threading::Lock l__inst( m_check );
      return *m_current;
    }
    
    Modtime const & modtime() {
      return m_modtime;
    }
    
    Modtime const & poke_modtime() {
      return m_poke_modtime;
    }
    
    Modtime const & poked() {
      m_poke_modtime = Modtime::modtime();
      //Master::master()->log( 1, "Poke modtime now " + m_poke_modtime.asString() );
      return m_poke_modtime;
    }
    
    void set_modtime( Modtime const & m ) {
      Infotrope::Threading::Lock l__inst( m_check );
      if( !m_paths.empty() || !m_set.empty() ) {
	throw std::runtime_error( "Attempt to set modtime on transaction which has pending items." );
      }
      m_modtime = m;
    }
    
    void clear() {
      Infotrope::Threading::Lock l__inst( m_check );
      m_current = 0;
      m_paths.clear();
      m_set.clear();
    }
    
    void rollback() {
      Datastore & ds( Datastore::datastore() );
      Infotrope::Threading::WriteLock l__inst( ds.lock() );
      ds.rollback();
      bool repeat( false );
      do {
	repeat = false;
	for ( list< Infotrope::Utils::magic_ptr<Subcontext> >::iterator i( m_set.begin() );
	      i != m_set.end(); i = m_set.begin() ) {
	  ( *i ) ->rollback();
	  m_set.erase( i );
	  repeat = true;
	}
	for( list< magic_ptr<Dataset> >::iterator i( m_paths.begin() );
	      i != m_paths.end(); i = m_paths.begin() ) {
	  (*i)->rollback();
	  m_paths.erase( i );
	  repeat = true;
	}
      } while ( repeat );
    }
    
    void commit() {
      Datastore & ds( Datastore::datastore() );
      Infotrope::Threading::WriteLock l__inst( ds.lock() );
      if( ds.commit() ) {
	trans().need_full();
      }
      bool repeat( false );
      std::list< Infotrope::Utils::magic_ptr<Dataset> > post_needed;
      do {
	repeat = false;
	for( list< Infotrope::Utils::magic_ptr<Subcontext> >::iterator i( m_set.begin() );
	      i != m_set.end(); i = m_set.begin() ) {
	  (*i)->commit( m_modtime );
	  m_set.erase( i );
	  repeat = true;
	}
	for( list< magic_ptr<Dataset> >::iterator i( m_paths.begin() );
	      i != m_paths.end(); i = m_paths.begin() ) {
	  (*i)->commit( m_modtime );
	  post_needed.push_back( ( *i ) );
	  m_paths.erase( i );
	  repeat = true;
	}
      } while ( repeat );
      for( std::list< Infotrope::Utils::magic_ptr<Dataset> >::const_iterator i( post_needed.begin() );
	    i != post_needed.end(); ++i ) {
	(*i)->post_commit( *i, m_modtime );
      }
      do {
	repeat = false;
	for( std::list< Infotrope::Utils::weak_ptr<Sink> >::const_iterator i( m_cxts_pending.begin() );
	     i != m_cxts_pending.end(); ++i ) {
	  m_cxts.push_back( *i );
	}
	m_cxts_pending.clear();
	for ( std::list< Infotrope::Utils::weak_ptr<Sink> >::const_iterator i( m_cxts.begin() );
	      i != m_cxts.end(); ++i ) {
	  Infotrope::Utils::magic_ptr<Sink> h( (*i).harden() );
	  if( h ) {
	    h->process_all();
	  }
	}
	m_cxts.clear();
	if( !m_cxts_pending.empty() ) {
	  repeat = true;
	}
      } while( repeat );
    }
    
    void add( Infotrope::Utils::magic_ptr<Subcontext> const & s ) {
      m_set.push_back( s );
    }
    void add( Infotrope::Utils::magic_ptr<Dataset> const & p ) {
      m_paths.push_back( p );
    }
    void add( Infotrope::Utils::weak_ptr<Sink> const & p ) {
      {
	Infotrope::Utils::magic_ptr<Sink> pp( p.harden() );
	if( pp ) {
	  if( 0!=dynamic_cast<TransLogger*>( pp.ptr() ) ) {
	    return;
	  }
	}
      }
      m_cxts.push_back( p );
    }
  private:
    Infotrope::Threading::Mutex m_main;
    Infotrope::Threading::Mutex m_check;
    pthread_t m_who;
    list< magic_ptr<Subcontext> > m_set;
    list< magic_ptr<Dataset> > m_paths;
    list< weak_ptr<Sink> > m_cxts;
    list< weak_ptr<Sink> > m_cxts_pending;
    Transaction * m_current;
    Modtime m_modtime;
    Modtime m_poke_modtime;
  };
  
  TransactionInstance & transaction_inst() {
    static TransactionInstance t;
    return t;
  }
  
  bool full_dump() {
    static unsigned long counter( 0 );
    if( ++counter == 10 ) {
      counter = 0;
      return true;
    }
    return false;
  }
}

Transaction::Transaction()
  : m_lock( transaction_inst().lock() ), m_translog(), m_state( InProgress ), m_record( true ), m_record_full( false ) {
  transaction_inst().set( this );
}

Transaction::Transaction( bool record )
  : m_lock( transaction_inst().lock() ), m_state( InProgress ), m_record( record ), m_record_full( false ) {
  transaction_inst().set( this );
}

Transaction::~Transaction() {
  if ( m_state != Complete ) {
    rollback();
  }
  transaction_inst().clear();
}

void Transaction::commit() {
  m_state = Commit;
  transaction_inst().commit();
  m_state = Complete;
  if( m_record ) {
    Master::master()->log( 1, std::string("Recording transaction, with record_full==") + (m_record_full?"true":"false") + " and translog " + (m_translog?"available":"not available") + "." );
    try {
      if( m_record_full || !m_translog ) {
	Dump d;
	d.dumper();
      } else {
	Master::master()->log( 1, "Performing translog dump." );
	m_translog->dumper( false, true );
	Master::master()->log( 1, "Done, hoorah!" );
	if( full_dump() ) { // Force a backgrounded full dump after every ten transactions.
	  Master::master()->log( 1, "Performing background full dump." );
	  Master::master()->full_dump();
	  Master::master()->log( 1, "Done background dump request." );
	}
      }
    } catch( std::exception & e ) {
      throw std::runtime_error( std::string( "Transaction recording failed, data stored but not logged: " ) + e.what() );
    }
  } else {
    Master::master()->log( 1, "Committing transaction, not recording." );
  }
}

void Transaction::set_modtime( Modtime const & m ) {
  if( m_state!=InProgress ) {
    throw std::runtime_error( "Whoops, this transaction is not in progress, but Modtime was set." );
  }
  transaction_inst().set_modtime( m );
}

void Transaction::rollback() {
  transaction_inst().rollback();
}

bool Transaction::mine() {
  return transaction_inst().mine();
}

void Transaction::add( Infotrope::Utils::magic_ptr<Subcontext> const & s ) {
  if ( !transaction_inst().mine() ) {
    throw std::runtime_error( "No transaction in progress, but add subcontext requested." );
  }
  transaction_inst().add( s );
}

void Transaction::add( Infotrope::Utils::magic_ptr<Dataset> const & p ) {
  if ( !transaction_inst().mine() ) {
    throw std::runtime_error( "No transaction in progress, but add dataset requested." );
  }
  if( !transaction().m_translog ) {
    transaction().m_translog = new TransLogger;
  }
  p->register_sink( Infotrope::Utils::magic_cast<Notify::Sink>( transaction().m_translog ) );
  transaction_inst().add( p );
}

void Transaction::add( Path const & p ) {
  if ( !transaction_inst().mine() ) {
    throw std::runtime_error( "No transaction in progress, but add dataset requested." );
  }
  add( Datastore::datastore().dataset( p ) );
}

void Transaction::add( weak_ptr<Sink> const & p ) {
  if ( !transaction_inst().mine() ) {
    throw std::runtime_error( "No transaction in progress, but add dataset requested." );
  }
  transaction_inst().add( p );
}

Transaction & Transaction::transaction() {
  if ( !mine() ) {
    throw std::runtime_error( "No transaction in progress, but transaction requested." );
  }
  return transaction_inst().trans();
}

Modtime const & Transaction::modtime() {
  if ( !mine() ) {
    throw std::runtime_error( "No transaction in progress, but modtime requested." );
  }
  return transaction_inst().modtime();
}
Modtime const & Transaction::poke_modtime() {
  if ( !mine() ) {
    throw std::runtime_error( "No transaction in progress, but modtime requested." );
  }
  return transaction_inst().poke_modtime();
}
Modtime const & Transaction::poked() {
  if ( !mine() ) {
    throw std::runtime_error( "No transaction in progress, but modtime requested." );
  }
  return transaction_inst().poked();
}
