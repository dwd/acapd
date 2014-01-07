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
#ifndef INFOTROPE_DATASTORE_CRITERIA_HH
#define INFOTROPE_DATASTORE_CRITERIA_HH

#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/sort.hh>
#include <infotrope/datastore/dataset.hh>

namespace Infotrope {
  
  namespace Data {
    
    namespace Criteria {
      typedef Utils::magic_ptr<Token::Token> t_tok;
      typedef Utils::magic_ptr<Data::Attribute> t_attr;
      typedef Utils::magic_ptr<Comparators::Comparator> t_comp;
      typedef Utils::magic_ptr<Data::Entry> t_entry;
      
      typedef Subcontext::const_iterator t_iter;
      typedef std::list < std::pair<t_iter,t_iter> > t_iter_range;
      
      class Criterion {
      public:
	Criterion();
	virtual ~Criterion() {
	}
	  
	// Say whether a particular iterator is acceptable.
	virtual bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & i, unsigned long int pos ) const = 0;
	
	// Give the acceptable range. Falls back to above if we can't
	// cheat somehow.
	virtual t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter_range const & ) const;
	// Can we derive an acceptable range quickly with this subcontext?
	// Default is false.
	virtual bool perform_range( Subcontext const & ) const;
	
	// Generate an optimal sort, for when searching through datasets.
	virtual Utils::magic_ptr<Sort> fast_sort() const;
      };
      
      class All : public Criterion {
      public:
	All();
	
	// Always yes.
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & i, unsigned long int pos ) const;
	// Always pair<begin,end>
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	// Always true.
	bool perform_range( Subcontext const & ) const;
      };
      
      class BinaryComp : public Criterion {
      protected:
	Utils::StringRep::entry_type const m_attr;
	t_attr const m_key;
	t_comp m_comp;
	mutable Utils::magic_ptr<Dataset> m_last_dset;
	bool m_noinherit;
	mutable bool m_default;
	mutable bool m_setup_default;
	Right const & m_search_right;
      public:
	BinaryComp( Utils::StringRep::entry_type const & at, std::string const & comp, t_tok const & tok, bool noinherit );
	
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & i, unsigned long int pos ) const;
	
	virtual bool acceptable_int( t_attr const & ) const = 0;
      };
      
      class Equal : public BinaryComp {
      public:
	Equal( Utils::StringRep::entry_type const & at, std::string const & comp, t_tok const & tok, bool );
	virtual t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	virtual bool acceptable_int( t_attr const & a ) const;
	virtual bool perform_range( Subcontext const & ) const;
      private:
	bool m_octet;
      };
      
      class CompareStrict : public BinaryComp {
      public:
	CompareStrict( Utils::StringRep::entry_type const & at, std::string const & m_comp, t_tok const & tok, bool );
	virtual bool acceptable_int( t_attr const & a ) const;
      };
      
      class Compare : public BinaryComp {
      public:
	Compare( Utils::StringRep::entry_type const & at, std::string const & m_comp, t_tok const & tok, bool );
	virtual bool acceptable_int( t_attr const & a ) const;
      };
      
      class Prefix : public BinaryComp {
      public:
	Prefix( Utils::StringRep::entry_type const & at, std::string const & m_comp, t_tok const & tok, bool );
	virtual bool acceptable_int( t_attr const & a ) const;
      };
      
      class Substring : public BinaryComp {
      public:
	Substring( Utils::StringRep::entry_type const & at, std::string const & m_comp, t_tok const & tok, bool );
	virtual bool acceptable_int( t_attr const & a ) const;
      };
      
      class Not : public Criterion {
      protected:
	Utils::magic_ptr < Criterion > m_crit;
      public:
	Not( Utils::magic_ptr < Criterion > const & c );
	
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & e, unsigned long int pos ) const;
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter_range const & e ) const;
	// Returns inverted range.
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	// Passes onto child.
	bool perform_range( Subcontext const & ) const;
      };
      
      class And : public Criterion {
      protected:
	Utils::magic_ptr < Criterion > m_crit1;
	Utils::magic_ptr < Criterion > m_crit2;
      public:
	And( Utils::magic_ptr < Criterion > const &, Utils::magic_ptr < Criterion > const & );
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & e, unsigned long int pos ) const;
	// Returns AND ranges.
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	// True if both children are also true.
	bool perform_range( Subcontext const & ) const;
      };
      
      class Or : public Criterion {
      protected:
	Utils::magic_ptr < Criterion > m_crit1;
	Utils::magic_ptr < Criterion > m_crit2;
      public:
	Or( Utils::magic_ptr < Criterion > const &, Utils::magic_ptr < Criterion > const & );
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & e, unsigned long int pos ) const;
	// Returns OR ranges.
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	// True if both children are also true.
	bool perform_range( Subcontext const & ) const;
      };
      
      // This was discintly ikky to do.
      // Only operates when the subcontext its given is exactly the
      // same as the subcontext everything else is working on.
      class Range : public Criterion {
      protected:
	long unsigned int m_from;
	long unsigned int m_to;
	
      public:
	Range( long unsigned int from, long unsigned int to );
	bool acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter const & e, unsigned long int pos ) const;
	// Returns AND range.
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const &, t_iter_range const & ) const;
	t_iter_range acceptable( Utils::StringRep::entry_type const & login, Subcontext const & ) const;
	// True.
	bool perform_range( Subcontext const & ) const;
      };
    }
  }
}


#endif
