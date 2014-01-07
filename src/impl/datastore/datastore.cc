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
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/exceptions.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/server/token.hh>
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/transaction.hh>

#include <infotrope/server/master.hh>

using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope;
using namespace Infotrope::Utils;

using namespace Infotrope::Server;

namespace {
  std::string const c_top( "/" );
  Datastore * s_ds(0);
}

Datastore::Datastore()
  : m_mutex(), m_datasets() {
  if( s_ds ) {
    throw std::runtime_error( "Datastore is singleton." );
  }
  s_ds = this;
}

Datastore::~Datastore() {
  Master::master()->log( 8, "Datastore being shutdown." );
  s_ds = 0;
}

magic_ptr<Dataset> const & Datastore::top() const {
  t_datasets::const_iterator top( m_datasets.find( c_top ) );
  return ( *top ).second;
}

magic_ptr<Dataset> const & Datastore::dataset( Path const & pathx ) const {
  pathx.validate();
  Path path( pathx.canonicalize() );
  //Master::master()->log( 1, "Pulling dataset " + path.asString() );
  if ( Transaction::mine() ) {
    {
      t_datasets::const_iterator ds( m_pending.find( path.asString() ) );
      if ( ds != m_pending.end() ) {
	if ( ( *ds ).second ) {
	  if( (*ds).second->exists2( c_entry_empty, true ) ) {
	    return ( *ds ).second;
	  }
	}
	//throw *(int *)0;
	Master::master()->log( 8, "No dataset while in transaction:" + path.asString() );
	throw Infotrope::Data::Exceptions::no_dataset( path.asString() );
      }
    }
  }
  t_datasets::const_iterator ds( m_datasets.find( path.asString() ) );
  if ( ds == m_datasets.end() ) {
    //throw *(int *)0;
    //Master::master()->log( 8, "No dataset committed: " + path.asString() );
    throw Infotrope::Data::Exceptions::no_dataset( path.asString() );
  }
  if( (*ds).second->exists2( c_entry_empty, true ) ) {
    return ( *ds ).second;
  } else {
    //Master::master()->log( 8, "Dataset being deleted." );
    throw Infotrope::Data::Exceptions::no_dataset( path.asString() );
  }
}

bool Datastore::exists( Path const & pathx ) const {
  pathx.validate();
  return exists_real( pathx.canonicalize() );
}

bool Datastore::exists_real( Path const & path ) const {
  //Master::master()->log( 1, "Looking for " + path.asString() );
  if ( Transaction::mine() ) {
    t_datasets::const_iterator ds( m_pending.find( path.asString() ) );
    if ( ds != m_pending.end() ) {
      if ( ( *ds ).second ) {
	//Master::master()->log( 1, "Found in transaction." );
	return (*ds).second->exists2( c_entry_empty, true );
      }
      //Master::master()->log( 1, "Found deleted in transaction." );
      return false;
    }
  }
  if( m_datasets.find( path.asString() ) != m_datasets.end() ) {
    //Master::master()->log( 1, "Found committed." );
    return ( *m_datasets.find( path.asString() ) ).second->exists2( c_entry_empty, true );
  } else {
    //Master::master()->log( 1, "Not found." );
    return false;
  }
}

// Notify the datastore that a dataset has changed name.
void Datastore::rename( Path const & oldpathx, Path const & newpathx ) {
  TRANSACTION_VALIDATE();
  oldpathx.validate();
  newpathx.validate();
  Path oldpath( oldpathx.canonicalize() );
  Path newpath( newpathx.canonicalize() );
  if ( !oldpath.translatable( Path::ByOwner ) ) {
    throw Exceptions::invalid_path( oldpath.asString(), "Cannot rename non-translatable path." );
  }
  if ( oldpath.parent() != newpath.parent() ) {
    throw Exceptions::invalid_path( oldpath.asString(), "Cannot move datasets." );
  }
  if ( oldpath.translate( Path::ByOwner ).mybit() != oldpath.translate( Path::ByClass ).mybit() ) {
    throw Exceptions::invalid_path( oldpath.asString(), "Cannot rename system datasets." );
  }
  rename_priv( oldpath, newpath );
}

void Datastore::rename_priv( Path const & oldpath, Path const & newpath ) {
  magic_ptr < Dataset > d( dataset( oldpath ) );
  d->path( newpath );
  d->inherit_reset();
  m_pending[ newpath.asString() ] = d;
  m_pending[ oldpath.asString() ] = magic_ptr < Dataset > ();
  Transaction::add( newpath );
  
  for ( Dataset::const_iterator i( d->begin() );
	i != d->end(); ++i ) {
    // Find entries with subdataset with "." value.
    if ( (*i).first->exists( c_attr_subdataset ) ) {
      // If we have a value of "." present, we should remember this one.
      Utils::magic_ptr<Comparators::Comparator> comp( Comparators::Comparator::comparator( "i;octet" ) );
      if ( comp->equals( (*i).first->attr( c_attr_subdataset ), magic_ptr<Attribute>( new Attribute( c_entry_empty, magic_ptr<Token::Token>( new Token::String( "." ) ) ) ) ) ) {
	rename_priv( oldpath.translate( Path::ByClass ).asString() + ( *i ).second.get(), newpath.translate( Path::ByClass ).asString() + ( *i ).second.get() );
      }
    }
  }
}

Utils::magic_ptr<Dataset> const & Datastore::create( Path const & path, bool addentries ) {
  Path p( path.canonicalize() );
  return create( p, Utils::magic_ptr<Subcontext>( new Subcontext( "cache:" + p.asString(), Transaction::modtime() ) ), Utils::magic_ptr<Subcontext>( new Subcontext( "req:" + p.asString(), Transaction::modtime() ) ), Utils::magic_ptr<Subcontext>( new Subcontext( p.asString(), Transaction::modtime() ) ), addentries );
}

Utils::magic_ptr<Dataset> const & Datastore::create_virtual( Path const & path ) {
  //throw std::string( "Datastore::create_virtual called for " + path.asString() );
  Utils::magic_ptr<Dataset> dset( create( path, false ) );
  Utils::magic_ptr<Entry> e( new Entry );
  e->add( new Attribute( c_attr_dataset_type, "virtual" ) );
  dset->add2( e, c_entry_empty );
  //Master::master()->log( 1, std::string("Finished creating virtual, dataset is ") + ( dset->isNormal() ? "Normal" : "Virtual" ) );
  return dataset( path );
}

void Datastore::add_subdataset( Path const & ppath, Utils::StringRep::entry_type const & name ) const {
  Utils::magic_ptr<Entry> e( new Entry );
  Utils::magic_ptr<Dataset> ds( dataset( ppath.asString() ) ); {
    Token::List * l( new Token::PList );
    l->add( new Token::String( "." ) );
    e->add( new Attribute( c_attr_subdataset, magic_ptr<Token::Token>( l ) ) );
  }
  Utils::magic_ptr<Entry> orig( ds->fetch2( name, true ) );
  if ( orig ) {
    if ( orig->exists( c_attr_subdataset ) ) {
      Utils::magic_ptr<Attribute> a( orig->attr( c_attr_subdataset )->clone() );
      Utils::magic_ptr<Comparators::Comparator> comp( Comparators::Comparator::comparator( "i;octet" ) );
      if ( comp->equals( e->attr( c_attr_subdataset ), magic_ptr<Attribute>( new Attribute( c_entry_empty, magic_ptr<Token::Token>( new Token::String( "." ) ) ) ) ) ) {
	return;   // Done already.
      }
      
      
      a->add( "." );
      e->add( a );
    }
  }
  ds->add2( e, name );
}

Utils::magic_ptr<Dataset> const & Datastore::create( Path const & path, Utils::magic_ptr<Subcontext> const & sc, Utils::magic_ptr<Subcontext> const & req, Utils::magic_ptr<Subcontext> const & ov, bool addentries ) {
  TRANSACTION_VALIDATE();
  path.validate();
  //Master::master()->log( 1, "Creation of " + path.asString() + " requested." );
  if ( exists_real( path ) ) {
    //Master::master()->log( 1, "Exists already." );
    if ( dataset( path.asString() )->isNormal() ) {
      // Normal dataset, always say it exists.
      return dataset( path );
    } else {
      if ( addentries ) {
	dataset( path )->fetch2( c_entry_empty, true )->store_nil( c_attr_dataset_type );
	create( path.parent(),  true );
	add_subdataset( path.parent(), path.mybit() );
      }
      return dataset( path );
    }
  }
  Master::master()->log( 1, "Need to create " + path.asString() );
  if ( path == c_top ) {
    m_pending[ c_top ] = magic_ptr<Dataset>( new Dataset( *this, c_top, sc, req, ov ) );
    m_pending[ c_top ]->setup();
    return dataset( c_top );
  } else if ( path == "/byowner/" ) {
    if ( !exists( c_top ) ) {
      create( c_top );
    }
    //Master::master()->log( 1, "Creating /byowner/" );
    m_pending[ "/byowner/" ] = magic_ptr<Dataset>( new Dataset( *this, std::string( "/byowner/" ), sc, req, ov ) );
    //Master::master()->log( 1, "Setup of byowner..." );
    m_pending[ "/byowner/" ]->setup();
    //Master::master()->log( 1, "Done." );
    if ( addentries ) add_subdataset( c_top, path.mybit() );
    //Master::master()->log( 1, "Added subdataset stuff." );
  } else {
    if ( ( ++( path.begin() ) ) == path.end() ) {
      // Possibly quicker than .size()==1
      // We have a new dataset-class, apparently.
      //Master::master()->log( 1, "Creating dataset-class " + path.mybit().get() );
      m_pending[ path.asString() ] = magic_ptr<Dataset>( new Dataset( *this, path, sc, req, ov ) );
      //Master::master()->log( 1, "Setup of " + path.mybit().get() + "..." );
      m_pending[ path.asString() ]->setup();
      //Master::master()->log( 1, "Done." );
      if ( addentries ) add_subdataset( c_top, path.mybit() );
      //Master::master()->log( 1, "Added subdataset stuff." );
    } else {
      Path ppath( path.parent() );
      // Now have the ppath.
      //Master::master()->log( 1, "Checking parent " + ppath.asString() );
      if ( !exists( ppath ) ) {
	//Master::master()->log( 1, "Need to create parent." );
	create( ppath, addentries );
	//Master::master()->log( 1, "Done, now back with " + path.asString() );
      }
      if( path.canonicalize()==path ) {
	//Master::master()->log( 1, "Creating..." );
	m_pending[ path.asString() ] = magic_ptr<Dataset>( new Dataset( *this, path, sc, req, ov ) );
	m_pending[ path.asString() ]->setup();
	//Master::master()->log( 1, "Setup..." );
      }
      //Master::master()->log( 1, "subdataset values..." );
      if ( addentries ) add_subdataset( ppath, path.mybit() );
      // Next, look in the alternate view.
      //Master::master()->log( 1, "Looking for alternate namespace dataset." );
      if ( path.isView( Path::ByOwner ) ) {
	if ( path.translatable( Path::ByClass ) ) {
	  create( path.translate( Path::ByClass ), sc, req, ov, addentries );
	}
      } else {
	if ( path.translatable( Path::ByOwner ) ) {
	  create( path.translate( Path::ByOwner ), sc, req, ov, addentries );
	}
      }
      //Master::master()->log( 1, "Done alt, now returning " + path.asString() );
    }
  }
  return dataset( path );
}


// stripthing should be false for virtuals, true for everything else.
// Although to be honest, I've forgotten what it's there for at all.
// And I've nooped it, now.
void Datastore::erase_final( Path const & p, bool stripthing ) {
  if( m_pending.find( p.asString() ) != m_pending.end() || m_datasets.find( p.asString() ) != m_datasets.end() ) {
    m_pending[ p.asString() ] = magic_ptr<Dataset>();
  }
  Master::master()->log( 5, "Erase final for " + p.asString() );
  
  Path pp( p.parent() );
  if ( exists( pp ) ) {
    if ( dataset( pp )->exists2( p.mybit(), true ) ) {
      Utils::magic_ptr < Entry > e( new Entry );
      if( stripthing ) {
	e->store_nil( c_attr_subdataset );
      } else {
	e->store_default( c_attr_subdataset );
      }
      dataset( pp )->add2( e, p.mybit() );
    }
  }
  
  if( p.translatable( Path::ByClass ) ) {
    Path px( p.translate( Path::ByClass ) );
    Path pp2( px.parent().canonicalize() );
    if( pp2 != pp ) {
      Utils::magic_ptr<Entry> e( new Entry );
      if( stripthing ) {
	e->store_nil( c_attr_subdataset );
      } else {
	e->store_default( c_attr_subdataset );
      }
      dataset( pp2 )->add2( e, px.mybit() );
    }
  }
}

void Datastore::erase( Path const & px, bool stripthing ) {
  Path p( px.canonicalize() );
  Utils::magic_ptr < Dataset > d2( dataset( p ) );
  Master::master()->log( 5, "Erasing " + p.asString() );
  if ( d2->erase() ) {
    erase_final( p );
  }
  // Remove the parent's subdataset entry, if there is one.
}

bool Datastore::commit() {
  bool did_stuff( false );
  for ( t_datasets::iterator i( m_pending.begin() );
	i != m_pending.end(); i = m_pending.begin() ) {
    did_stuff = true;
    if ( !( *i ).second ) {
      t_datasets::iterator i2( m_datasets.find( ( *i ).first ) );
      if ( i2 != m_datasets.end() ) {
	m_datasets.erase( i2 );
      }
    } else {
      m_datasets.insert( *i );
    }
    m_pending.erase( i );
  }
  return did_stuff;
}

void Datastore::rollback() {
  m_pending.clear();
}

Datastore & Datastore::datastore() {
  if( s_ds ) {
    return *s_ds;
  }
  throw std::runtime_error( "Datastore not initialized." );
}

Datastore const & Datastore::datastore_read() {
  return datastore();
}

// Iterator stuff.

Datastore::const_iterator::const_iterator( t_master m, t_datasets const * mm, t_datasets const * p )
  : m_encountered(), m_master( m ), m_datasets( mm ), m_pending( p ), m_intrans( true ), m_scan_trans( false ) {
  if( m==mm->end() ) {
    m_scan_trans = true;
    m_master = p->end();
  }
}
Datastore::const_iterator::const_iterator( t_master m, t_datasets const * mm )
  : m_encountered(), m_master( m ), m_datasets( mm ), m_pending( 0 ), m_intrans( false ), m_scan_trans( false ) {
}

Datastore::const_iterator::t_value const & Datastore::const_iterator::operator*() const {
  if( !m_scan_trans ) {
    if( m_intrans ) {
      t_master tmp( m_pending->find( (*m_master).first ) );
      if( tmp!=m_pending->end() ) {
	return (*tmp);
      }
    }
  }
  return *m_master;
}

Datastore::const_iterator & Datastore::const_iterator::operator++() {
  bool just_changed( false );
  //Master::master()->log( 1, "++ on iterator..." );
  if( !m_scan_trans ) {
    //Master::master()->log( 1, "++ :: ++m_master" );
    ++m_master;
    if( m_master == m_datasets->end() ) {
      //Master::master()->log( 1, "++ :: Hit end of committed list." );
      if( m_intrans ) {
	//Master::master()->log( 1, "++ :: Ah-ha, in transaction!" );
	m_master = m_pending->begin();
	m_scan_trans = true;
	just_changed = true;
      }
    } else {
      if( m_intrans ) {
	t_master m( m_pending->find( (*m_master).first ) );
	if( m!=m_pending->end() && !((*m).second) ) {
	  return this->operator++();
	}
      }
    }
  }
  if( m_scan_trans ) {
    //Master::master()->log( 1, "++ :: Scanning transaction." );
    if( !just_changed ) {
      //Master::master()->log( 1, "++ :: ++m_master here." );
      ++m_master;
    }
    while( m_master!=m_pending->end() ) {
      if( (*m_master).second ) {
	//Master::master()->log( 1, "++ :: Found valid transaction entry, returning." );
	break;
      }
      //Master::master()->log( 1, "++ :: Transaction entry is dummy, skipping." );
      ++m_master;
    }
  }
  //Master::master()->log( 1, "++ :: RETURN" );
  return *this;
}

Datastore::const_iterator Datastore::begin() const {
  if( Transaction::mine() ) {
    return Datastore::const_iterator( m_datasets.begin(), &m_datasets, &m_pending );
  } else {
    return Datastore::const_iterator( m_datasets.begin(), &m_datasets );
  }
}

Datastore::const_iterator Datastore::end() const {
  if( Transaction::mine() ) {
    return Datastore::const_iterator( m_datasets.end(), &m_datasets, &m_pending );
  } else {
    return Datastore::const_iterator( m_datasets.end(), &m_datasets );
  }
}
