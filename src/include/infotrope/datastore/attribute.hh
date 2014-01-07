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
#ifndef INFOTROPE_DATASTORE_ATTRIBUTE_HH
#define INFOTROPE_DATASTORE_ATTRIBUTE_HH

#include <string>
#include <infotrope/server/token.hh>
#include <infotrope/utils/stringrep.hh>
#include <infotrope/datastore/acl.hh>
#include <infotrope/datastore/path.hh>
#ifdef _REENTRANT
#include <infotrope/threading/lock.hh>
#endif

namespace Infotrope {
  namespace Comparators {
    class Comparator;
  }
  namespace Data {
    
    class Attribute {
    public:
      typedef Token::Token t_val;
      typedef Utils::magic_ptr<t_val> t_ptr;
      typedef Utils::StringRep::entry_type t_name;
      Attribute( t_name const &, t_ptr const & );
      Attribute( t_name const &, std::string const & );
      Attribute( t_name const & );   // Value is NIL.
      Attribute( t_name const &, Utils::magic_ptr<Acl> const & ); // Value is an ACL.
      
      std::string const & attribute() const {
	return m_attribute.get();
      }
      Utils::StringRep::entry_type const & attribute_rep() const {
	return m_attribute;
      }
      Utils::magic_ptr<Acl> const & acl() const {
	return m_acl;
      }
      void acl( Acl const & a ) {
	m_acl = Utils::magic_ptr<Acl> ( new Acl( a ) );
	m_active_acl = m_acl;
      }
      void acl( Utils::magic_ptr<Acl> const & a ) {
	m_acl = a;
	m_active_acl = a;
      }
      t_ptr const & value() const {
	return m_value;
      }
      Utils::magic_ptr<Acl> const & value_acl() const {
#if 0
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	if( !m_value_acl ) {
	  m_value_acl = new Acl( m_value );
	}
#endif
	return m_value_acl;
      }
      Utils::StringRep::entry_type const & value_rep() {
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	if( !m_value_rep.ptr() ) {
	  m_value_rep = Utils::StringRep::entry_type( valuestr() );
	}
	return m_value_rep;
      }
	
      Utils::magic_ptr<Acl> active_acl( Utils::magic_ptr<Acl> const & a ) {
	return m_active_acl = a;
      }
      Utils::magic_ptr<Acl> const & active_acl() const {
	return m_active_acl;
      }
      void value( t_ptr const & v ) {
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	m_transform.clear();
	m_value = v;
	m_value_acl.zap();
	m_size = 0;
      }
      void value_acl( Utils::magic_ptr<Acl> const & av ) {
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	m_transform.clear();
	m_value = av->tokens();
	m_value_acl = av;
	m_size = 0;
      }
      t_ptr const & value_transform( int magic ) {
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	return m_transform[magic];
      }
      t_ptr const & value_transform( int magic, t_ptr const & tr ) {
#ifdef _REENTRANT
	Infotrope::Threading::Lock l__inst( m_lock );
#endif
	return m_transform[magic] = tr;
      }
      t_ptr const & size() {
	if ( m_size.ptr() == 0 ) {
	  size_calc();
	}
	return m_size;
      }
      void size_calc();
      std::string const & valuestr() const {
	try {
	  return m_value->toString().value();
	} catch( std::string const & e ) {
	  throw std::string( "While in "+m_attribute.get()+" : "+e );
	}
      }
      Utils::magic_ptr<Attribute> clone() const;
      void add( Utils::magic_ptr<Token::String> const & );
      void add( std::string const & s ) {
	add( Utils::magic_ptr<Token::String>( new Token::String( s ) ) );
      }
      // Used only for non-notify contexts, and in Act.
      Utils::magic_ptr<RightList> myrights() const {
	return m_myrights;
      }
      void myrights( Utils::magic_ptr<RightList> const & r ) {
	m_myrights = r;
      }
      
      RightList myrights( Utils::StringRep::entry_type const& user, Utils::StringRep::entry_type const & owner ) {
	if( !m_active_acl ) {
	  throw std::runtime_error( "No active ACL!" );
	} else {
	  return m_active_acl->myrights( user, owner );
	}
      }
      
      Path const & origin() const {
	return m_origin;
      }
      void origin( Path const & o ) {
	m_origin = o;
      }
      
    private:
      t_ptr m_value;
      Utils::magic_ptr<Acl> m_value_acl;
      t_ptr m_size;
      t_name m_attribute;
      Utils::StringRep::entry_type m_value_rep;
      Utils::magic_ptr<Acl> m_acl;
      Utils::magic_ptr<Acl> m_active_acl;
      Path m_origin;
      Utils::magic_ptr<RightList> m_myrights;
      std::vector<t_ptr> m_transform;
#ifdef _REENTRANT
      Infotrope::Threading::Mutex m_lock;
#endif
    };
  }
}

#endif
