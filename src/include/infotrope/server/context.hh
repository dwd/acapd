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
#ifndef INFOTROPE_DATASTORE_CONTEXT_HH
#define INFOTROPE_DATASTORE_CONTEXT_HH

#include <infotrope/threading/siglock.hh>
#include <infotrope/threading/rw-lock.hh>
// #include "thread.hh"
// #include "acap-worker.hh"
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/criteria.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/server/return.hh>
#include <infotrope/datastore/notify-source.hh>
#include <infotrope/datastore/notify-sink.hh>

namespace Infotrope {
  namespace Server {
    
    class Worker;
    
    /*class ContextMessage {
    private:
      Utils::magic_ptr<Data::Dataset> m_dset;
      Utils::StringRep::entry_type m_entry;
      Data::Modtime m_modtime;
    public:
      ContextMessage( Utils::magic_ptr < Data::Dataset > const & dset, Utils::StringRep::entry_type const & entry, Data::Modtime const & modtime );
      Data::Modtime const & modtime() const {
	return m_modtime;
      }
      bool process( Context * ) const;
      };*/
    
    /*class DatasetInfo {
    public:
      Path virtual_path; // Path to present to user. Not user expanded.
      Path logical_path; // Path without resolution, so non-immediate local datasets would have different path here than their own. Also ByClass namespace would be different here.
      bool is_virtual; // True if this is a virtual dataset.
      };*/
    
    class Context : public Notify::Sink, public Notify::Source {
      // Typedefs
    public:
      // The following map is map[ dataset_pointer ] => pair< DisplayPath, map[ LogicalPath ] => depth >.
      // The problem is:
      /*
	Let's suppose we have a simple bookmarks dataset, containing a bunch of links and folders.
	It's at /bookmarks/group/shared/, and contains something called
	/bookmarks/group/shared/foo/banana.
	
	Easy so far.
	
	It gets mentioned in /bookmarks/~/bar, and also in /bookmarks/~/zab/wiggle, as
	a subdataset.
	
	Let's suppose we do a search, creating a notify context, with depth 3.
	
	Let's now assume that we delete /bookmarks/~/bar.
	At this point, banana shouldn't be listed anymore, since it exceeds the depth, but
	foo still should be.
	
	What needs to happen is that during the search, we note that the dataset has multiple
	logical paths, even though we only list the entries once.
	
	When bar gets deleted, we also run through all the datasets, and delete any logical
	paths relating to it. If we deleted the last logical path for a dataset, we need to remove
	all the entries.
	
	Frankly, this is a lot of work.
	
	Addendum:
	
	I later decided that the standard doesn't actually say this at all.
	
	Or, more accurately, it didn't mean to.
	
	The relevant bit is in the DEPTH search modifier text. It says that REFER intermediate responses
	are only generated for datasets "not located on the current server", which implies that DEPTH should
	follow non-immediate subdatasets. Roughly the same thing is said in section 6.4.4, incidentally.
	
	However, and this is telling, it also specifies that the REFER "payload" includes one or more
	*relative* subdatasets.
	
	Now, if you're trying to, for instance, build a menu in the client for your bookmarks, or build some tree
	representation of your addressbook heirarchy, the idea of following all local subdataset 'references'
	sounds great. Until you try to figure out where they need to go, and what depth to use on remote
	searches. It's painful, since you then have to work out why the server gave you the data, in order to
	simply guess the number.
	
	If you only follow immediate subdatasets on the server, then you always know the depth - just count
	the '/' characters.
	
	However, following all local datasets can be very useful for addressbook searches, where you generally
	don't care overly about the depth, but do care greatly about finding all possible addressbook entries
	very quickly. So I've left all the code in, but allowed the client to specify which behaviour it needs, via
	the XFOLLOW search modifier.
	
	All of this is not helped at all by the minor problem that the specification *never* makes mention of a
	subdataset attribute containing anything other than '.' or a remote ACAP URL, yet never proscribes
	against it.
	
	Oh, I also left the follow code in because it was a fuckload of work, and I couldn't bring myself to strip
	it entirely. :-)
      */
      typedef std::map< Data::Path /* logical path */, unsigned int /* depth */> t_dset_entry;
      typedef std::map< Utils::magic_ptr<Data::Dataset>, std::pair< Utils::magic_ptr<Data::Path>, t_dset_entry > > t_dset_list;
      // Data Members
    private:
      std::string const m_name;
      Utils::magic_ptr<Data::Subcontext> m_subcontext;
      Utils::magic_ptr<Data::Subcontext> m_sdset_subcontext;
      Utils::magic_ptr<Data::Criteria::Criterion> m_crit;
      Utils::magic_ptr<Return> m_return ;
      Utils::weak_ptr<Worker> m_worker;
      Threading::Mutex m_mutex;
      //::Thread::SigMutex m_message_signal;
      Threading::RWMutex m_subcxt_mutex;
      bool m_shutdown;
      t_dset_list m_used_datasets;
      //std::list < Utils::magic_ptr < ContextMessage > > m_messages;
      bool m_notify;
      bool m_enumerate;
      bool m_persist;
      bool m_noinherit;
      std::string m_base_search;
      unsigned long int m_depth;
      Data::Modtime m_modtime;
      bool m_follow_local;
      
    public:
      Context( std::string const &,
	       Utils::magic_ptr<Data::Subcontext> const &,
	       Utils::magic_ptr<Data::Subcontext> const &,
	       Utils::magic_ptr<Data::Criteria::Criterion> const &,
	       Utils::magic_ptr<Return> const &,
	       t_dset_list const &,
	       std::string base_search,
	       unsigned long int depth,
	       bool notify,
	       bool enumerate,
	       bool persist,
	       bool noinherit,
	       bool follow_local,
	       Worker & );
      
      bool isNotify() const {
	return m_notify;
      }
      bool persist() const {
	return m_persist;
      }
      
      void setup();
      
      void updatecontext();
      Data::Modtime const & modtime() const {
	return m_modtime;
      }
      
      std::string const & name() const {
	return m_name;
      }
      Data::Path const & first_dataset() const;
      
      //void message( Utils::magic_ptr<ContextMessage> const & );
      
      Utils::magic_ptr<Data::Subcontext> subcontext_pure() const {
	return m_subcontext;
      }
      
      Threading::RWMutex const & cxt_lock() const {
	return m_subcxt_mutex;
      }
      
      void shutdown();
      Threading::Mutex & lock() {
	return m_mutex;
      }
      void signal( Threading::Lock & );
      
      Utils::magic_ptr<Data::Dataset> find_best_sdset( Utils::magic_ptr<Data::Dataset> const & dset, Utils::magic_ptr<Data::Entry> const & );
      bool subdataset_check( Utils::magic_ptr<Data::Dataset> const & dset, Utils::StringRep::entry_type const &, Utils::StringRep::entry_type const &, Utils::magic_ptr<Data::Entry> const &, Utils::magic_ptr<Data::Entry> const &, Data::Modtime const & );
      bool change( Utils::magic_ptr<Data::Dataset> const & dset, Utils::StringRep::entry_type const & entry, Data::Modtime const & );
      void handle_notify( Utils::magic_ptr<Source> const &, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which ) throw();
      void handle_complete() throw();
      void bind( Worker & );
      void unbind();
    };
    
  }
}

#endif
