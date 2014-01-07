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

#include <infotrope/datastore/authz.hh>
#include <infotrope/datastore/constants.hh>

#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/attribute.hh>
#include <infotrope/datastore/path.hh>

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>

using namespace Infotrope::Data;
using namespace Infotrope::Utils::StringRep;
using namespace Infotrope::Constants;
using namespace Infotrope;

namespace {
  Utils::StringRep::entry_type extract_realm( entry_type const & foo ) {
    std::string::size_type at( foo->find( '@' ) );
    if( at==std::string::npos ) {
      return ""; // Magic empty realm.
    }
    return foo->substr( at+1 );
  }
}
    

AuthZ::AuthZ( entry_type const & userid ) : m_userid( userid ), m_neg_userid( "-" + userid.get() ), m_realm( extract_realm( userid ) ) {
}

AuthZ::~AuthZ() {
}

AuthZ::member_type AuthZ::memberOf( entry_type const & id ) {
  Threading::Lock l__inst( m_mutex );
  t_group_cache::const_iterator i( m_pve.find( id ) );
  if( i != m_pve.end() ) {
    return (*i).second;
  }
  return m_pve[ id ] = get_memberOf( id );
}

AuthZ::member_type AuthZ::negativeMemberOf( entry_type const & id ) {
  Threading::Lock l__inst( m_mutex );
  t_group_cache::const_iterator i( m_nve.find( id ) );
  if( i != m_nve.end() ) {
    return (*i).second;
  }
  return m_nve[ id ] = get_negativeMemberOf( id );
}

AuthZ::member_type AuthZ::get_memberOf( entry_type const & id ) {
  if( id == c_authid_anyone ) {
    return Everybody;
  } else if( id == m_userid ) {
    return Equivalent;
  } else if( !id->empty() && id.get()[0]=='/' ) {
    // Group.
    if( id->length()>=2 ) {
      // Group with possible magic character.
      switch( id.get()[1] ) {
      case ':':
	{
	  // UNIX group.
	  char p[1024];
	  struct group gbuf, *gbufp;
	  getgrnam_r( id->c_str() + 2, &gbuf, p, 1024, &gbufp );
	  if( !gbufp ) return NotMember; // Group doesn't exist.
	  for( char ** px( gbuf.gr_mem ); px; ++px ) {
	    if( m_userid.get() == *px ) {
	      return InGroup;
	    }
	  }
	}
	break;
      case '@':
	if( m_realm == id->substr( 2 ) ) {
	  return InGroup;
	}
	break;
      default:
	{
	  try {
	    Path p( id.get() );
	    Path bc( p.translate( Path::ByClass ) );
	    if( (*(bc.asList().begin())) == "groupid" ) {
	      Threading::ReadLock l__inst( Datastore::datastore_read().lock() );
	      Datastore const & ds( Datastore::datastore_read() );
	      Utils::magic_ptr<Dataset> dset( ds.dataset( p ) );
	      for( Dataset::const_iterator i( dset->begin() ); i!=dset->end(); ++i ) {
		Utils::magic_ptr<Entry> e( (*i).first );
		if( e->exists( "groupid.user.member" ) ) {
		  Utils::magic_ptr<Token::Token> val( e->attr( "groupid.user.member" )->value() );
		  if( val->isString() ) {
		    if( m_userid == val->toString().value() ) {
		      return InGroup;
		    }
		  }
		}
	      }
	    }
	  } catch( std::exception & ) {
	  }
	}
      }
    }
  }
  return NotMember;
}

AuthZ::member_type AuthZ::get_negativeMemberOf( entry_type const & id ) {
  if( !id->empty() && id.get()[0]=='-' ) {
    return get_memberOf( id->substr( 1 ) );
  }
  return NotMember;
}

Infotrope::Utils::magic_ptr<AuthZ> AuthZ::authz( entry_type const & userid ) {
  return Infotrope::Utils::magic_ptr<AuthZ>( new AuthZ( userid ) );
}
