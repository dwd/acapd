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
#ifndef INFOTROPE_DATASTORE_DATASTORE_HH
#define INFOTROPE_DATASTORE_DATASTORE_HH

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/threading/rw-lock.hh>
#include <infotrope/datastore/path.hh>
#include <infotrope/datastore/dataset.hh>
#include <list>
#include <map>
#include <string>

namespace Infotrope {
  
  namespace Data {
      
    class Datastore {
    public:
      typedef enum {
	Normal = 0,
	// The path is '/', or there exists a parent dataset which is also normal, and contains an entry named by the last path component with a subdataset value including "."
	Virtual = 1,
	// The path is effectively a redirection, but always followed by default. The path does not exist if NOINHERIT is specified. A new dataset created at this path should create a dataset which inherits from the real path this points to.
	Logical = 1<<1
	// The path is produced by following entries with subdataset values which are local, but may not include "."
      } path_type_t;
      Datastore();
      ~Datastore();
    private:
      Datastore( Datastore const & );
    public:
      
      Infotrope::Utils::magic_ptr<Dataset> const & top() const;
      Infotrope::Utils::magic_ptr<Dataset> const & dataset( Path const & ) const;
      Infotrope::Utils::magic_ptr<Dataset> const & dataset( Path const &, path_type_t ) const;
      bool exists( Path const & ) const;
    private:
      bool exists_real( Path const & ) const;
    public:
      
      void rename( Path const &, Path const & );
      Infotrope::Utils::magic_ptr<Dataset> const & create( Path const &, bool = true );
      Infotrope::Utils::magic_ptr<Dataset> const & create_virtual( Path const & );
    private:
      Infotrope::Utils::magic_ptr<Dataset> const & create( Path const &, Infotrope::Utils::magic_ptr<Subcontext> const &, Infotrope::Utils::magic_ptr<Subcontext> const &, Infotrope::Utils::magic_ptr<Subcontext> const &, bool );
    public:
      void erase( Path const &, bool = true );
    private:
      void erase_final( Path const &, bool = true );
    public:
      
      static Datastore & datastore();
      static Datastore const & datastore_read();
      
      Infotrope::Threading::RWMutex & lock() {
	return m_mutex;
      }
      Infotrope::Threading::RWMutex const & lock() const {
	return m_mutex;
      }
      
      void rollback();
      bool commit(); // Returns true if a complete rewrite is needed.
	
      typedef std::map< Utils::StringRep::entry_type, Infotrope::Utils::magic_ptr < Dataset >, Utils::StringRep::entry_type_less > t_datasets;
      
      /*typedef t_datasets::const_iterator const_iterator;
      const_iterator begin() const {
	return m_datasets.begin();
      }
      const_iterator end() const {
	return m_datasets.end();
	}*/
      
      class const_iterator {
      public:
	typedef std::pair< Utils::StringRep::entry_type const, Infotrope::Utils::magic_ptr<Dataset> > t_value;
	typedef t_datasets::const_iterator t_master;
	
	const_iterator( t_master m, t_datasets const * q );
	const_iterator( t_master m, t_datasets const * q, t_datasets const * x );
	
	t_value const & operator *() const;
	const_iterator & operator++();
	bool operator!=( const_iterator const & x ) const {
	  if( x.m_intrans==m_intrans ) {
	    if( x.m_scan_trans==m_scan_trans ) {
	      return m_master!=x.m_master;
	    }
	  }
	  return true;
	}
      private:
	std::set<Utils::StringRep::entry_type> m_encountered;
	t_master m_master;
	t_datasets const * m_datasets;
	t_datasets const * m_pending;
	bool m_intrans;
	bool m_scan_trans;
      };
      const_iterator begin() const;
      const_iterator end() const;
      
    private:
      void add_subdataset( Path const &, Infotrope::Utils::StringRep::entry_type const & ) const;
      void rename_priv( Path const &, Path const & );
      Infotrope::Threading::RWMutex m_mutex;
      t_datasets m_datasets;
      t_datasets m_pending;
      typedef std::pair< Utils::StringRep::entry_type, Utils::weak_ptr<Dataset> > t_last_hit;
      Infotrope::Utils::magic_ptr<t_last_hit> m_last;
      Infotrope::Utils::magic_ptr<t_last_hit> m_last_trans;
      //t_datasets m_virtuals;
      //t_datasets m_virtuals_pending;
    };
    
  }
    
}

#endif
