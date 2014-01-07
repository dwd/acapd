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
#ifndef INFOTROPE_DATASTORE_TRANSACTION_HH
#define INFOTROPE_DATASTORE_TRANSACTION_HH

#include <infotrope/threading/lock.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <sstream>
#include <string>
#include <infotrope/datastore/path.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/notify-sink.hh>
#include <infotrope/datastore/trans-logger.hh>

#define TRANSACTION_VALIDATE() if( !Transaction::mine() ) { \
std::ostringstream ss; \
ss <<"No transaction in progress at " << __FILE__ << ":" << __LINE__; \
throw std::string( ss.str() ); \
}

namespace Infotrope {
  namespace Data {
    class Subcontext;
    class Dataset;
    // Really TransactionHandle, but that would be a messy name
    // involving far too much typing.
    class Transaction {
    public:
      typedef enum {
	InProgress,
	Commit,
	Complete
      } state;
      
      Transaction();
      Transaction( bool );
      ~Transaction();
      
      static bool mine();
      static void add( Utils::magic_ptr<Subcontext> const & );
      static void add( Utils::magic_ptr<Dataset> const & );
      static void add( Utils::weak_ptr<Notify::Sink> const & );
      static void add( Path const & );
      static Transaction & transaction();
      static Modtime const & modtime();
      static Modtime const & poke_modtime();
      static Modtime const & poked();
      
      void set_modtime( Modtime const & );
      
      void commit();
      void rollback();
      bool record() const {
	return m_record;
      }
      void need_full() {
	m_record_full = true;
      }
    private:
      Infotrope::Threading::Lock m_lock;
      Utils::magic_ptr<TransLogger> m_translog;
      state m_state;
      bool m_record;
      bool m_record_full;
    };
    
  }
  
}

#endif
