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
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/transaction.hh>
#define SUBCXT_DEBUG
#ifdef SUBCXT_DEBUG
#include <infotrope/server/master.hh>
#define LOG( x ) Infotrope::Server::Master::master()->log( 1, (x) )
#else
#define LOG( x ) 0
#endif
using namespace std;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope::Utils;
using namespace Infotrope;



Subcontext::Subcontext() : m_clear( false ), m_dirty( false ), m_loaded( false ) {
}

Subcontext::Subcontext( std::string const & name ) : m_clear( false ), m_name( name ), m_dirty( false ), m_loaded( false ) {
}

Subcontext::Subcontext( Sort const & s )
  : m_sort( s ), m_ordered( m_sort ), m_clear( false ), m_dirty( false ), m_loaded( false ) {
}                       

Subcontext::Subcontext( std::string const & name, Modtime const & m )
  : m_clear( false ), m_name( name ), m_modtime( m ), m_dirty( false ), m_loaded( false ) {
}

void Subcontext::clear() {
  TRANSACTION_VALIDATE();
  m_pending.clear();
  m_clear = true;
}

bool Subcontext::exists2( Utils::StringRep::entry_type const & entry ) const {
  //cout << "Searching for entry '" << entry.get() << "'.\n";
  if ( Transaction::mine() ) {
    //cout << "  In transaction.\n";
    t_contents_pending::const_iterator i( m_pending.find( entry ) );
    if ( m_pending.end() != i ) {
      if ( ( *i ).second ) {
        //cout << "  Transaction has it.\n";
        return true;
      } else {
        //cout << "  Entry deleted in transaction.\n";
        return false;
      }
    }
    if ( m_clear ) return false;
  }
  rebuild();
  t_contents::const_iterator i( m_contents.find( entry ) );
  if( i==m_contents.end() ) {
    return false;
  }
  return true;
}

namespace {
  static inline Utils::magic_ptr<Entry> const & s_empty() {
    static Utils::magic_ptr<Entry> e;
    return e;
  }
}

Utils::magic_ptr<Entry> const & Subcontext::fetch2( Utils::StringRep::entry_type const & name, bool failok ) const {
  if ( Transaction::mine() ) {
    t_contents_pending::const_iterator i( m_pending.find( name ) );
    if ( m_pending.end() != i ) {
      if ( ( *i ).second ) {
        return ( *i ).second;
      } else {
        if( failok ) return s_empty();        
        throw std::runtime_error( "No such entry in '" + m_name + "'" );
      }
    }
    if ( m_clear ) {
      if( failok ) return s_empty();
      throw std::runtime_error( "No such entry in '" + m_name + "'" );
    }
  }
  t_contents::const_iterator i( m_contents.find( name ) );
  if( i==m_contents.end() ) {
    if( failok ) return s_empty();
    throw std::runtime_error( "No such entry : '" + name.get() + "' in '" + m_name + "'" );
  }
  return ( *( (*i).second ) ).first;
}

Subcontext::const_iterator Subcontext::fetch_iterator( Utils::StringRep::entry_type const & name ) const {
  rebuild();
  if ( m_contents.find( name ) == m_contents.end() ) {
    throw std::runtime_error( "No such entry : '" + name.get() + "' in '" + m_name + "', or not committed." );
  }
  return ( *( m_contents.find( name ) ) ).second;
}

void Subcontext::add( Utils::StringRep::entry_type const & n, Utils::magic_ptr < Entry > const & e ) {
  TRANSACTION_VALIDATE();
  //std::cout << "Adding (impure) with modtime " << Transaction::modtime().asString() << "\n";
  //e->add( new Attribute( c_attr_modtime, Transaction::modtime().asString() ) );
  add_pure( n, e );
}

void Subcontext::add_pure( Utils::StringRep::entry_type const & n, Utils::magic_ptr < Entry > const & e ) {
  TRANSACTION_VALIDATE();
  //cout << "Adding (pure) entry '" << n.get() << "'\n";
  //if ( !e->exists( c_attr_modtime ) ) {
  //  add( n, e );
  //  return ;
  //}
  m_pending[ n ] = e;
}

void Subcontext::add_notrans( Utils::StringRep::entry_type const & n, Utils::magic_ptr < Entry > const & e ) {
  if( !e ) {
    throw std::runtime_error( "Cannot insert NULL entry." );
  }
  remove_notrans( n );
  std::pair<iterator,bool> tmp( m_ordered.insert( make_pair( e, n ) ) );
  if ( tmp.second ) {
    m_contents[ n ] = tmp.first;
  } else {
    throw std::runtime_error( "Argh! Already inserted?" );
  }
  m_rebuild_index = true;
  m_dirty = true;
}

void Subcontext::remove( Utils::StringRep::entry_type const & n ) {
  TRANSACTION_VALIDATE();
  m_pending[ n ] = Utils::magic_ptr<Entry>();
}

void Subcontext::remove_notrans( Utils::StringRep::entry_type const & n ) {
  t_contents::iterator x( m_contents.find( n ) );
  if ( x != m_contents.end() ) {
    iterator i( ( *x ).second );
    m_ordered.erase( i );
    m_contents.erase( x );
  }
  m_rebuild_index = true;
  m_dirty = true;
}

Subcontext::const_iterator Subcontext::begin() const {
  return m_ordered.begin();
}

Subcontext::const_iterator Subcontext::end() const {
  return m_ordered.end();
}

Subcontext::size_type Subcontext::size() const {
  return m_ordered.size();
}

void Subcontext::commit( Modtime const & m ) {
  //std::cout << "Committing subcontext '" << m_name << "'.\n";
  if ( m_clear ) {
    //std::cout << "Clearing subcontext totally.\n";
    m_ordered.clear();
    m_contents.clear();
    m_clear = false;
  }
  for ( t_contents_pending::iterator i( m_pending.begin() );
        i != m_pending.end(); i = m_pending.begin() ) {
    //std::cout << "Committing pending Entry '" << ( *i ).first.get() << "'\n";
    t_contents::iterator j( m_contents.find( ( *i ).first ) );
    if ( j != m_contents.end() ) {
      m_ordered.erase( ( *j ).second );
      m_contents.erase( j );
    }
    //std::cout << "Erased old data.\n";
    if ( ( *i ).second ) {
      //std::cout << "Adding new Entry '" << ( *i ).first.get() << "'\n";
      //magic_ptr < Attribute > modtime( new Attribute( "modtime", m.asString() ) ); ;
      add_notrans( ( *i ).first, ( *i ).second );
    }
    //std::cout << "Erasing pending Entry.\n";
    m_pending.erase( i );
  }
  rebuild();
  //std::cout << "Finished committing subcontext.\n";
  if( Transaction::transaction().record() ) {
    m_modtime = m;
  } else {
    m_dirty = false;
  }
}

void Subcontext::rollback() {
  m_clear = false;
  m_dirty = false;
  m_pending.clear();
}

// Old experiments, long outdated.

void Subcontext::rebuild() {
}

void Subcontext::rebuild() const {
}
