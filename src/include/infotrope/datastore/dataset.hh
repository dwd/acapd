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
#ifndef INFOTROPE_DATASTORE_DATASET_HH
#define INFOTROPE_DATASTORE_DATASET_HH

#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/path.hh>
#include <infotrope/server/token.hh>
#include <infotrope/datastore/acl.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/notify-source.hh>

namespace Infotrope {
  
  namespace Data {
    
    class Datastore;
    class Dataset : public Notify::Source {
    public:
      Dataset( Datastore &, Path const & path, Infotrope::Utils::magic_ptr< Subcontext > const &, Infotrope::Utils::magic_ptr<Subcontext> const &, Infotrope::Utils::magic_ptr< Subcontext > const & ov );
      void setup();
      
      void store( Infotrope::Utils::magic_ptr<Token::List> const &, Infotrope::Utils::StringRep::entry_type const & user );
      void add2( Infotrope::Utils::magic_ptr<Entry> const &, Infotrope::Utils::StringRep::entry_type const &, bool=false );
      void add2( Infotrope::Utils::magic_ptr<Entry> const & );
      void add2( Entry * e ) {
	add2( Infotrope::Utils::magic_ptr<Entry>( e ) );
      }
      Modtime const & last_deletion() const {
	return m_last_deletion;
      }
      void poke( Infotrope::Utils::StringRep::entry_type const & );
      bool poke_child( Infotrope::Utils::StringRep::entry_type const & );
      bool poke_data( Infotrope::Utils::StringRep::entry_type const & );
      void rollback();
      void commit( Modtime const & );
      bool erase();   // Erase. If it returns false, then don't *actually* erase it.
      void post_commit( Utils::magic_ptr<Dataset> const &, Modtime const & );
      void inherit_reset();
      void inherit_recalculate();
      void inherit_set( Path const & );
      void inherit_clear();
      Utils::magic_ptr<Acl> acl( Infotrope::Utils::StringRep::entry_type const & entry, Infotrope::Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const &, Utils::magic_ptr<Attribute> const & ) const;
      RightList myrights( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::StringRep::entry_type const & user ) const;
      RightList myrights( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::StringRep::entry_type const & user, Utils::magic_ptr<Entry> const &, Utils::magic_ptr<Attribute> const & ) const;
      
      void path( Path const & path );
      Path const & path() const;
      
      Infotrope::Utils::magic_ptr<Entry> const & fetch2( Infotrope::Utils::StringRep::entry_type const & e, bool noinherit=false ) const;
      bool exists2( Infotrope::Utils::StringRep::entry_type const & e, bool noinherit=false ) const;
      
      bool visible( bool noinherit ) const;
      bool isNormal();
      
      Modtime const & modtime() const {
	return m_overlay_subcontext->modtime();
      }
      std::string const & source() const {
	return m_overlay_subcontext->source();
      }
      
      typedef Subcontext::iterator iterator;
      typedef Subcontext::const_iterator const_iterator;
      
      iterator begin( bool noinherit=false );
      iterator end( bool noinherit=false );
      const_iterator begin( bool noinherit=false ) const;
      const_iterator end( bool noinherit=false ) const;
      
      Utils::magic_ptr<Subcontext> & subcontext_pure( bool noinherit=false );
      
      Utils::magic_ptr<Subcontext> & requested_subcontext() {
	return m_requested_subcontext;
      }
    private:
      Utils::magic_ptr<Acl> acl_find( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const &, Utils::magic_ptr<Attribute> const & ) const;
      Utils::StringRep::entry_type acl_get_owner( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const &, Utils::magic_ptr<Attribute> const & ) const;
      
      Utils::magic_ptr<Dataset> dataset_inherit() const;
      
    private:
      Utils::magic_ptr<Subcontext> m_requested_subcontext;
      Utils::magic_ptr<Subcontext> m_subcontext;
      Utils::magic_ptr<Subcontext> m_overlay_subcontext;
      Path m_path;
      Path m_trans_path;
      Datastore & m_datastore;
      bool m_inherit_changed;
      bool m_deletion;
      Modtime m_last_deletion;
      typedef std::set<Utils::StringRep::entry_type, Utils::StringRep::entry_type_less> t_poked;
      t_poked m_poked;
      bool m_erasing;
      Utils::magic_ptr<Entry> m_empty;
      Utils::magic_ptr<Entry> m_overlay_empty;
      Utils::magic_ptr<Entry> m_requested_empty;
      mutable std::map< Utils::StringRep::entry_type, std::map< Utils::StringRep::entry_type, Infotrope::Utils::magic_ptr<Acl>, Utils::StringRep::entry_type_less >, Utils::StringRep::entry_type_less > m_acl_cache;
    };
  }
}

#endif
