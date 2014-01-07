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
#ifndef INFOTROPE_DATASTORE_SUBCONTEXT_HH
#define INFOTROPE_DATASTORE_SUBCONTEXT_HH

#include <string>
#include <map>
#include <list>
#include <set>

#include <infotrope/datastore/sort.hh>

#include <infotrope/utils/magic-ptr.hh>

#include <infotrope/utils/stringrep.hh>

#include <infotrope/datastore/modtime.hh>

namespace Infotrope {
  
  namespace Data {
    
    class Subcontext {
    public:
      typedef std::map< Utils::StringRep::entry_type, Utils::magic_ptr < Entry > , Utils::StringRep::entry_type_less > t_contents_pending;
      typedef std::pair<Utils::magic_ptr<Entry>, Utils::StringRep::entry_type> t_payload;
      typedef std::pair<Utils::magic_ptr<Entry> const, Utils::StringRep::entry_type> t_payload_const;
      typedef std::set<t_payload, Sort> t_contents_ordered;
      typedef t_contents_ordered::iterator iterator;
      typedef t_contents_ordered::const_iterator const_iterator;
      typedef t_contents_ordered::size_type size_type;
      typedef std::map < Utils::StringRep::entry_type, iterator, Utils::StringRep::entry_type_less > t_contents;
      
      Subcontext();
      // Null store, default sort, never saved.
      
      Subcontext( std::string const & path );
      // This store path, default sort, never saved.
      
      Subcontext( Sort const & );
      // Null store, this sort, never saved.
      
      Subcontext( std::string const & path, Modtime const & m );
      // This store path, saved last at m.
      
      Subcontext( std::string const & path, Modtime const & m, Sort const & );
      Subcontext( std::string const & path, Sort const & );
      
      bool exists2( Utils::StringRep::entry_type const & ) const;
      
      Utils::magic_ptr<Entry> const & fetch2( Utils::StringRep::entry_type const &, bool failok=false ) const;
      const_iterator fetch_iterator( Utils::StringRep::entry_type const & ) const;
      
      void add( Utils::StringRep::entry_type const &, Utils::magic_ptr < Entry > const & );
      void add_pure( Utils::StringRep::entry_type const &, Utils::magic_ptr < Entry > const & );
      void clear();
      
      void add_notrans( Utils::StringRep::entry_type const &, Utils::magic_ptr < Entry > const & );
      void remove( Utils::StringRep::entry_type const & );
      
      void remove_notrans( Utils::StringRep::entry_type const & );
      
      void rollback();
      void commit( Modtime const & m );
      Modtime const & modtime() const {
	return m_modtime;
      }
      std::string const & source() const {
	return m_name;
      }
      void rebuild();
      void rebuild() const;
      
      const_iterator begin() const;
      const_iterator end() const;
      size_type size() const;
      
      Sort const & sort() const {
	return m_sort;
      }
      
    private:
      Sort m_sort;
      t_contents m_contents;
      t_contents_pending m_pending;
      t_contents_ordered m_ordered;
      bool m_clear;
      bool m_rebuild_index;
      std::string m_name;
      Modtime m_modtime;
      bool m_dirty;
      bool m_loaded;
    };
    
  }
  
}

#endif
