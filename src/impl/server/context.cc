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
 *  Thu Feb 13 12:53:37 2003
 *  Copyright  2003 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/
#include <infotrope/server/context.hh>
#include <infotrope/server/worker.hh>
#include <infotrope/server/master.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/server/token.hh>
#include <infotrope/threading/rw-lock.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/path.hh>
#include <infotrope/datastore/entry.hh>

#include <sstream>

using namespace Infotrope::Server;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope;
using namespace Infotrope::Utils;
using namespace std;

Context::Context( std::string const & e,
		  Utils::magic_ptr<Subcontext> const & a,
		  Utils::magic_ptr<Subcontext> const & sdsets,
		  Utils::magic_ptr<Criteria::Criterion> const & b,
		  Utils::magic_ptr<Return> const & c,
		  t_dset_list const & f,
		  std::string base_search,
		  long unsigned int depth,
		  bool notify,
		  bool enumerate,
		  bool persist,
		  bool noinherit,
		  bool follow_local,
		  Worker & d )
  : m_name( e ), m_subcontext( a ), m_sdset_subcontext( sdsets ), m_crit( b ), m_return( c ), m_worker( Master::master()->get( d.fd() ) ), m_mutex(), m_shutdown( false ), m_used_datasets( f ), m_notify( notify ), m_enumerate( enumerate ), m_persist( persist ), m_noinherit( noinherit ), m_base_search( base_search ), m_depth( depth ), m_follow_local( follow_local ) {
}

Utils::magic_ptr<Dataset> Context::find_best_sdset( Utils::magic_ptr<Dataset> const & dset, Utils::magic_ptr<Entry> const & e ) {
  if( !e || !e->exists( c_attr_subdataset ) ) {
    return Utils::magic_ptr<Dataset>();
  }
  Entry::subdataset_info_t sdl( e->subdataset( dset->path() ) );
  for( Entry::subdataset_info_t::const_iterator i( sdl.begin() ); i!=sdl.end(); ++i ) {
    if( (*i).original=="." ) {
      //Master::master()->log( 1, "Found immediate." );
      if( Datastore::datastore().exists( (*i).path ) ) {
	//Master::master()->log( 1, "Okay, using this." );
	return Datastore::datastore_read().dataset( (*i).path );
      }
    }
  }
  if( m_follow_local ) {
    //Master::master()->log( 1, "Looking for first non-remote dataset." );
    for( Entry::subdataset_info_t::const_iterator i( sdl.begin() ); i!=sdl.end(); ++i ) {
      //Master::master()->log( 1, "Considering "+(*i).path.asString() );
      if( !(*i).path.isRemote() ) {
	//Master::master()->log( 1, "Not remote." );
	if( Datastore::datastore().exists( (*i).path ) ) {
	  //Master::master()->log( 1, "Yes, exists." );
	  return Datastore::datastore_read().dataset( (*i).path );
	}
      }
    }
  }
  return Utils::magic_ptr<Dataset>();
}

bool Context::subdataset_check( Utils::magic_ptr<Dataset> const & dset,
				Utils::StringRep::entry_type const & ename,
				Utils::StringRep::entry_type const & myentryname,
				Utils::magic_ptr<Entry> const & eold,
				Utils::magic_ptr<Entry> const & enew,
				Modtime const & m ) {
  // First, see if the old and new have a subdataset.
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return false;
  Utils::magic_ptr<Dataset> dold( find_best_sdset( dset, eold ) );
  Utils::magic_ptr<Dataset> dnew;
  if( dset->myrights( ename, c_attr_subdataset, worker->login() ).have_right( 'r' ) ) {
    dnew = find_best_sdset( dset, enew );
  }
  if( dold.ptr()==dnew.ptr() ) {
    return false;
  }
  unsigned int parent_min_depth(0);
  Utils::magic_ptr<Path> opath;
  for( t_dset_entry::const_iterator i( m_used_datasets[dset].second.begin() ); i!=m_used_datasets[dset].second.end(); ++i ) {
    if( !parent_min_depth || parent_min_depth > (*i).second ) {
      parent_min_depth = (*i).second;
      opath = new Path( m_used_datasets[dset].first->asString() + ename.get() );
    }
  }
  //Master::master()->log( 1, "Here." );
  bool changed( false );
  if( dold ) {
    // Remove old dataset entries if needs be.
    // Find, copy, and remove the logical path entry.
    unsigned int child_min_depth(0);
    Path spath( *m_used_datasets[dold].first );
    m_used_datasets[dold].second.erase( *opath );
    for( t_dset_entry::const_iterator i( m_used_datasets[dold].second.begin() ); i!=m_used_datasets[dold].second.end(); ++i ) {
      if( !child_min_depth || child_min_depth > (*i).second ) {
	child_min_depth = (*i).second;
      }
    }
    if( m_used_datasets[dold].second.size()==0 ) {
      m_used_datasets.erase(dold);
    }
    if( child_min_depth==0 ) {
      // No paths left to this data, we should remove it.
      //Master::master()->log( 1, "No paths left to " + dold->path().asString() + " will remove it." );
      unsigned long int pos(1);
      for( Subcontext::iterator i( m_subcontext->begin() ); i!=m_subcontext->end(); ++i ) {
	//Master::master()->log( 1, "Considering " + (*i).second.get() );
	if( (*i).second->find( spath.asString() )==0 ) {
	  Utils::magic_ptr<Token::List> l( new Token::List );
	  l->add( "*" );
	  l->add( "REMOVEFROM" );
	  l->add( new Token::String( m_name ) );
	  l->add( new Token::String( (*i).second.get() ) );
	  l->add( new Token::Integer( pos ) );
	  worker->send( l, true );
	  Subcontext::iterator j( i );
	  ++j;
	  m_subcontext->remove_notrans( (*i).second );
	  --j;
	  i = j;
	  changed = true;
	} else {
	  ++pos;
	}
      }
    } else {
      // Hmmm... Some paths are left.
      if( m_depth ) {
	// And we have a minimum depth. We need to trim the tree, as it were.
	unsigned int plen( spath.size() - parent_min_depth );
	unsigned long int pos(1);
	for( Subcontext::iterator i( m_subcontext->begin() ); i!=m_subcontext->end(); ++i ) {
	  if( (*i).second->find( spath.asString() )==0 ) {
	    if( ( Path( (*i).second.get() ).size() - plen ) >= m_depth ) {
	      Utils::magic_ptr<Token::List> l( new Token::List );
	      l->add( "*" );
	      l->add( "REMOVEFROM" );
	      l->add( new Token::String( m_name ) );
	      l->add( new Token::String( (*i).second.get() ) );
	      l->add( new Token::Integer( m_enumerate?pos:0 ) );
	      worker->send( l, true );
	      Subcontext::iterator j( i );
	      ++j;
	      m_subcontext->remove_notrans( (*i).second );
	      --j;
	      i = j;
	      changed = true;
	      notify( magic_cast<Notify::Source>( worker->context( m_name ) ), myentryname, m, pos );
	    } else {
	      ++pos;
	    }
	  }
	}
      }
    }
  } else {
    m_sdset_subcontext->remove_notrans( myentryname );
  }
  //Master::master()->log( 1, "And here.1" );
  if( dnew ) {
    // Add new dataset entries if we need to.
    // Need to register sink here, too.
    // Also need to figure out probable logical path[s], display paths, etc.
    m_used_datasets[ dnew ].first = opath;
    m_used_datasets[ dnew ].second[ Path( myentryname.get() ) ] = parent_min_depth + 1;
    dnew->register_sink( Utils::magic_cast<Notify::Sink>( worker->context( m_name ) ) );
    m_sdset_subcontext->add_notrans( myentryname, enew );
    for( Dataset::iterator i( dnew->begin() ); i!=dnew->end(); ++i ) {
      //Master::master()->log( 1, "And here.2" );
      changed = change( dnew, (*i).second, m ) || changed;
    }
  }
  //Master::master()->log( 1, "And here.3" );
  return changed;
}

bool Context::change( Utils::magic_ptr<Dataset> const & dset,
		      Utils::StringRep::entry_type const & e,
		      Modtime const & m ) {
  //Master::master()->log( 1, "Processing context dataset change for " + e.get() );
  if ( m_shutdown ) {
    //Master::master()->log( 1, "Shutting down." );
    return false;
  }
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return false;
  //Master::master()->log( 1, "Got worker, time to carry on." );
  Utils::StringRep::entry_type myentryname( e );
  if ( m_depth!=1 ) {
    myentryname = Utils::StringRep::entry_type( m_used_datasets[dset].first->asString() + e.get() );
  }
  //Master::master()->log( 1, "Finding old position." );
  unsigned int oldpos(0);
  bool have_it( false );
  Utils::magic_ptr<Entry> eold;
  if( m_subcontext->exists2( myentryname ) ) {
    Subcontext::const_iterator i( m_subcontext->fetch_iterator( myentryname ) );
    //Master::master()->log( 1, "Okay, got iterator." );
    if( i!=m_subcontext->end() ) {
      have_it = true;
      eold = (*i).first;
      if ( m_enumerate ) {
	oldpos = 1;
	for ( ; i != m_subcontext->begin(); --i ) {
	  ++oldpos;
	}
      }
    }
  }
  if( !have_it && m_depth!=1 ) {
    eold = m_sdset_subcontext->fetch2( myentryname, true );
  }
  bool removefrom( false );
  bool changed( false );
  bool addto( false );
  Utils::magic_ptr<Entry> enew;
  //Master::master()->log( 1, "Checking status for " + m_name + ", entry '" + myentryname.get() + "'" );
  if( !dset->exists2( c_entry_empty, m_noinherit ) ) {
    removefrom = true;
  } if ( !dset->exists2( e, m_noinherit ) ) {
    removefrom = true;
    // It's not actually there anymore.
  } else if( !dset->myrights( c_entry_empty, c_attr_entry, worker->login() ).have_right( 'r' ) ) {
    removefrom = true;
  } else if ( !dset->myrights( e.get(), c_attr_entry, worker->login() ).have_right( 'r' ) ) {
    // We don't, so ignore it totally.
    removefrom = true;
  } else if( !dset->fetch2( e, m_noinherit )->exists( c_attr_entry ) ) {
    removefrom = true;
  } else {
    //Master::master()->log( 1, "Okay, exists and we have rights." );
    enew = dset->fetch2( e, m_noinherit );
    if( !m_crit || m_crit->acceptable( worker->login(), *(dset->subcontext_pure( m_noinherit )), dset->subcontext_pure( m_noinherit )->fetch_iterator( e ), 0 ) ) {
      //Master::master()->log( 1, "Adding..." );
      m_subcontext->add_notrans( myentryname, enew );
      addto = true;
    } else {
      //Master::master()->log( 1, "Failed criteria." );
      removefrom = true;
    }
    //Master::master()->log( 1, "Completed with entry." );
  }
  //Master::master()->log( 1, "Figuring out report type." );
  // Finally, find out how to report the change.
  bool change( false );
  if ( have_it ) {
    if( removefrom ) {
      //Master::master()->log( 1, "Removing old entry." );
      m_subcontext->remove_notrans( myentryname );
    }
    if( addto ) {
      addto = false;
      change = true;
    }
  } else {
    if( removefrom ) {
      //Master::master()->log( 1, "Entry to remove is not present, nothing to report." );
      return changed || m_depth==1 || subdataset_check( dset, e, myentryname, eold, enew, m );
    }
  }
  long unsigned int newpos( 0 );
  if ( m_enumerate && !removefrom ) {
    Subcontext::const_iterator i( m_subcontext->fetch_iterator( myentryname ) );
    newpos = 1;
    for ( ; i != m_subcontext->begin(); --i ) {
      ++newpos;
    }
  }
  //Master::master()->log( 1, "Cleaning response type." );
  if ( m_shutdown ) {
    //Master::master()->log( 1, "Shutting down, aborting change." );
    return false;
  }
  if( removefrom ) {
    change = false;
    addto = false;
  }
  if( change ) {
    addto = false;
  }
  //Master::master()->log( 1, "Sending report." );
  {
    Utils::magic_ptr < Token::List > l( new Token::List );
    l->add( new Token::Atom( "*" ) );
    if ( addto ) {
      l->add( new Token::Atom( "ADDTO" ) );
    } else if ( change ) {
      l->add( new Token::Atom( "CHANGE" ) );
    } else if ( removefrom ) {
      l->add( new Token::Atom( "REMOVEFROM" ) );
    }
    l->add( new Token::String( m_name ) );
    l->add( new Token::String( myentryname.get() ) );
    if ( change || removefrom ) {
      l->add( new Token::Integer( oldpos ) );
    }
    if ( change || addto ) {
      l->add( new Token::Integer( newpos ) );
    }
    if ( !removefrom ) {
      Subcontext::const_iterator i( m_subcontext->fetch_iterator( myentryname ) );
      if( (*i).second!=myentryname ) {
	throw std::runtime_error( "Arse! iterators don't match! : <<" + (*i).second.get() + "//" + myentryname.get() + ">>" );
      }
      if ( m_return ) {
	l->add( m_return->fetch_entry_metadata( dset.get(), (*i).first ) );
      }
    }
    //Master::master()->log( 1, "Sending..." );
    worker->send( l, false );
    //Master::master()->log( 1, "Notify..." );
    notify( magic_cast<Notify::Source>( worker->context( m_name ) ), myentryname, m, newpos );
    //Master::master()->log( 1, "Done." );
  }
  if( m_depth!=1 ) {
    subdataset_check( dset, e, myentryname, eold, enew, m );
  }
  return true;
}

void Context::setup() {
  //::Thread::Lock l__inst( m_mutex );
  if ( m_notify ) {
    Utils::magic_ptr<Worker> worker( m_worker.harden() );
    if( !worker ) return;
    std::ostringstream ss;
    //ss <<m_name << ": Have " << m_used_datasets.size() << " datasets to register with.";
    //Master::master()->log( 1, ss.str() );
    for ( t_dset_list::const_iterator i( m_used_datasets.begin() );
	  i != m_used_datasets.end(); ++i ) {
      //Master::master()->log( 1, m_name + "Registering sink with " + (*i).first->path().asString() );
      (*i).first->register_sink( magic_cast<Notify::Sink>( worker->context( m_name ) ) );
    }
    if( m_used_datasets.empty() ) {
      worker->context( m_base_search )->register_sink( magic_cast<Notify::Sink>( worker->context( m_name ) ) );
    }
  }
}

void Context::shutdown() {
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return;
  if ( m_notify ) {
    updatecontext();
    for ( t_dset_list::const_iterator i( m_used_datasets.begin() );
	  i != m_used_datasets.end(); ++i ) {
      (*i).first->unregister_sink( magic_cast<Notify::Sink>( worker->context( m_name ) ) );
    }
  }
  magic_ptr < Token::List > l( new Token::List );
  l->add( new Token::Atom( "*" ) );
  l->add( new Token::Atom( "OK" ) );
  l->add( new Token::QuotedString( "Context " + m_name + " ready for reaping." ) );
  worker->send( l, false );
}

void Context::updatecontext() {
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return;
  Threading::Lock l__inst( m_mutex );
  Utils::magic_ptr < Token::List > l( new Token::List );
  l->add( new Token::Atom( "*" ) );
  l->add( new Token::Atom( "MODTIME" ) );
  l->add( new Token::String( m_name ) );
  l->add( new Token::String( Modtime::modtime().asString() ) );
  worker->send( l, true );
}

void Context::handle_notify( Utils::magic_ptr<Source> const & s, Utils::StringRep::entry_type const & what, Modtime const & when, unsigned long int which ) throw() {
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return;
  try {
    if( m_modtime < when ) {
      m_modtime = when;
    }
    Dataset * d( dynamic_cast<Dataset*>( s.ptr() ) );
    if( d ) {
      Utils::magic_ptr<Dataset> dset( Utils::magic_cast<Dataset>( s ) );
      if( Datastore::datastore_read().exists( dset->path() ) ) {
	Utils::magic_ptr<Dataset> dset_current( Datastore::datastore_read().dataset( dset->path() ) );
	if( dset_current.ptr()==dset.ptr() || Path( m_base_search ).canonicalize()!=dset->path() ) {
	  change( dset, what, when );
	} else {
	  // Dataset has been deleted and recreated. Remove everything, then rescan.
	  //Master::master()->log( 1, "Context's base has been recreated..." );
	  for( Subcontext::iterator i( m_subcontext->begin() ); i!=m_subcontext->end(); ++i ) {
	    Utils::magic_ptr<Token::List> l( new Token::List );
	    l->add( "*" );
	    l->add( "REMOVEFROM" );
	    l->add( m_name );
	    l->add( (*i).second.get() );
	    l->add( new Token::Integer( m_enumerate?1:0 ) );
	    worker->send( l );
	  }
	  while( m_subcontext->begin()!=m_subcontext->end() ) {
	    m_subcontext->remove_notrans( (*m_subcontext->begin()).second );
	  }
	  // TODO :: rescan.
	}
      } else if( Path( m_base_search ).canonicalize()==dset->path() ) {
	// Dataset has been deleted, remove everything, then listen to parent for recreation. [Ugh]
	//Master::master()->log( 1, "Context's base has been removed..." );
	for( Subcontext::iterator i( m_subcontext->begin() ); i!=m_subcontext->end(); ++i ) {
	  Utils::magic_ptr<Token::List> l( new Token::List );
	  l->add( "*" );
	  l->add( "REMOVEFROM" );
	  l->add( m_name );
	  l->add( (*i).second.get() );
	  l->add( new Token::Integer( m_enumerate?1:0 ) );
	  worker->send( l );
	}
	while( m_subcontext->begin()!=m_subcontext->end() ) {
	  m_subcontext->remove_notrans( (*m_subcontext->begin()).second );
	}
	// TODO :: Listen to parent.
      }
    } else {
      if( m_subcontext->exists2( what ) ) {
	unsigned long ocount( 0 );
	if( m_enumerate ) {
	  Subcontext::const_iterator oi( m_subcontext->fetch_iterator( what ) );
	  do {
	    --oi;
	    ++ocount;
	  } while( m_subcontext->begin()!=oi );
	}
	m_subcontext->remove_notrans( what );
	// REMOVEFROM m_name ocount what.get()
	Utils::magic_ptr<Token::List> l( new Token::List );
	l->add( "*" );
	l->add( "REMOVEFROM" );
	l->add( m_name );
	l->add( what.get() );
	l->add( new Token::Integer( ocount ) );
	worker->send( l );
	notify( magic_cast<Notify::Source>( worker->context( m_name ) ), what, when, 0 );
      }
    }
  } catch( std::exception & e ) {
    Utils::magic_ptr<Token::List> l( new Token::List );
    l->add( "*" );
    l->add( "ALERT" );
    l->add( new Token::QuotedString( std::string("Context ") + e.what() ) );
    worker->send( l, true );
    Master::master()->log( 8, std::string("Context ") + e.what() );
  } catch( std::string & e ) {
    Utils::magic_ptr<Token::List> l( new Token::List );
    l->add( "*" );
    l->add( "ALERT" );
    l->add( new Token::QuotedString( std::string("Context [STR] ") + e ) );
    worker->send( l, true );
    Master::master()->log( 8, std::string("Context [STR] ") + e );
  }
}

void Context::handle_complete() throw() {
  Utils::magic_ptr<Worker> worker( m_worker.harden() );
  if( !worker ) return;
  try {
    if( m_base_search.empty() || m_base_search[0]!='/' ) {
      // Context stuff.
      if( worker->context_exists( m_base_search ) ) {
	Utils::magic_ptr<Subcontext> nsc( new Subcontext( m_subcontext->sort() ) );
	Utils::magic_ptr<Context> cxt( worker->context( m_base_search ) );
	unsigned long int count( 0 );
	for( Subcontext::const_iterator i( cxt->subcontext_pure()->begin() );
	     i!=cxt->subcontext_pure()->end(); ++i ) {
	  ++count;
	  if( !m_crit || m_crit->acceptable( worker->login(), *(cxt->subcontext_pure()), i, count ) ) {
	    nsc->add_notrans( (*i).second, (*i).first );
	    Utils::magic_ptr<Entry> rprt( m_subcontext->fetch2( (*i).second, true ) );
	    if( rprt ) {
	      bool report( false );
	      if( rprt.ptr() != (*i).first.ptr() ) {
		// CHANGE in value.
		report = true;
	      }
	      unsigned long ocount( 0 );
	      if( m_enumerate ) {
		Subcontext::const_iterator oi( m_subcontext->fetch_iterator( (*i).second ) );
		do {
		  --oi;
		  ++ocount;
		} while( oi!=m_subcontext->begin() );
	      }
	      m_subcontext->remove_notrans( (*i).second );
	      m_subcontext->add_notrans( (*i).second, (*i).first );
	      unsigned long ncount( 0 );
	      if( m_enumerate ) {
		Subcontext::const_iterator oi( m_subcontext->fetch_iterator( (*i).second ) );
		do {
		  --oi;
		  ++ncount;
		} while( oi!=m_subcontext->begin() );
		if( ncount!=ocount ) {
		  report = true;
		}
	      }
	      if( report ) {
		// CHANGE m_name ocount count (*i).second m_return->whatever( (*i).first )
		Utils::magic_ptr<Token::List> l( new Token::List );
		l->add( "*" );
		l->add( "CHANGE" );
		l->add( m_name );
		l->add( (*i).second.get() );
		l->add( new Token::Integer( ocount ) );
		l->add( new Token::Integer( ncount ) );
		Path source_dset( (*i).first->attr( c_attr_control )->valuestr() );
		l->add( m_return->fetch_entry_metadata( *( Datastore::datastore_read().dataset( source_dset ) ), ( *i ).first ) );
		worker->send( l );
		notify( magic_cast<Notify::Source>( worker->context( m_name ) ), (*i).second, m_modtime, ncount );
	      }
	    } else {
	      m_subcontext->add_notrans( (*i).second, (*i).first );
	      unsigned long ncount( 0 );
	      if( m_enumerate ) {
		Subcontext::const_iterator oi( m_subcontext->fetch_iterator( (*i).second ) );
		do {
		  --oi;
		  ++ncount;
		} while( oi!=m_subcontext->begin() );
	      }
	      // ADDTO m_name ncount (*i).second m_return->whatever( (*i).first )
	      Utils::magic_ptr<Token::List> l( new Token::List );
	      l->add( "*" );
	      l->add( "ADDTO" );
	      l->add( m_name );
	      l->add( (*i).second.get() );
	      l->add( new Token::Integer( ncount ) );
	      Path source_dset( (*i).first->attr( c_attr_control )->valuestr() );
	      l->add( m_return->fetch_entry_metadata( *( Datastore::datastore_read().dataset( source_dset ) ), ( *i ).first ) );
	      worker->send( l );
	      notify( magic_cast<Notify::Source>( worker->context( m_name ) ), (*i).second, m_modtime, ncount );
	    }
	  } else {
	    if( m_subcontext->exists2( (*i).second ) ) {
	      unsigned long ocount( 0 );
	      if( m_enumerate ) {
		Subcontext::const_iterator oi( m_subcontext->fetch_iterator( (*i).second ) );
		do {
		  --oi;
		  ++ocount;
		} while( m_subcontext->begin()!=oi );
	      }
	      // REMOVEFROM m_name ocount (*i).second
	      m_subcontext->remove_notrans( (*i).second );
	      Utils::magic_ptr<Token::List> l( new Token::List );
	      l->add( "*" );
	      l->add( "REMOVEFROM" );
	      l->add( m_name );
	      l->add( (*i).second.get() );
	      l->add( new Token::Integer( ocount ) );
	      worker->send( l );
	      notify( magic_cast<Notify::Source>( worker->context( m_name ) ), (*i).second, m_modtime, 0 );
	    }
	  }
	}
	m_modtime = cxt->modtime();
      } else {
	// Context gone away. Send REMOVEFROM for all entries.
	unsigned long ocount(0);
	while( m_subcontext->begin()!=m_subcontext->end() ) {
	  Subcontext::iterator i( m_subcontext->begin() );
	  ++ocount;
	  Utils::StringRep::entry_type n( (*i).second );
	  m_subcontext->remove_notrans( n );
	  Utils::magic_ptr<Token::List> l( new Token::List );
	  l->add( "*" );
	  l->add( "REMOVEFROM" );
	  l->add( m_name );
	  l->add( n.get() );
	  l->add( new Token::Integer( m_enumerate?ocount:0 ) );
	  worker->send( l );
	  notify( magic_cast<Notify::Source>( worker->context( m_name ) ), n.get(), m_modtime, 0 );	  
	}
      }
    }
    updatecontext();
  } catch( std::exception & e ) {
    Utils::magic_ptr<Token::List> l( new Token::List );
    l->add( "*" );
    l->add( "ALERT" );
    l->add( new Token::QuotedString( std::string("Context ") + e.what() ) );
    worker->send( l, true );
    Master::master()->log( 8, std::string("Context [2] ") + e.what() );
  } catch( std::string & e ) {
    Utils::magic_ptr<Token::List> l( new Token::List );
    l->add( "*" );
    l->add( "ALERT" );
    l->add( new Token::QuotedString( std::string("Context [STR] ") + e ) );
    worker->send( l, true );
    Master::master()->log( 8, std::string("Context [STR2] ") + e );
  }
}

void Context::bind( Worker & w ) {
  Utils::magic_ptr<Worker> wk( Master::master()->get( w.fd() ) );
  m_worker = wk; // Rebind weak pointer.
  processing( true ); // Allow processing of messages.
  process_all(); // Process all pending messages. Yeah, this is ropey in the extreme.
}

void Context::unbind() {
  processing( false );
}
