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
 *            acap-acl.cc
 *
 *  Thu Feb 13 12:49:47 2003
 *  Copyright  2003  Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/

#include <infotrope/datastore/acl.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/authz.hh>
#include <stdexcept>

using namespace Infotrope::Data;
using namespace std;
using namespace Infotrope::Token;
using namespace Infotrope::Utils;
using namespace Infotrope::Constants;

// #define DEBUGGING_ACL
#ifdef DEBUGGING_ACL
#include <infotrope/server/master.hh>
#define log( x ) Infotrope::Server::Master::master()->log( 1, x )
#else
#define log( x ) (void)0
#endif

Acl::Acl()
  : m_map(), m_isnil( true ) {}



Acl::Acl( magic_ptr<Token::Token> const & l )
  : m_map(), m_isnil( false ) {
  tokens( l );
}

void Acl::tokens( magic_ptr<Token::Token> const & l ) {
  if ( !l ) {
    m_isnil = true;
    return ;
  }
  if( l->isNil() ) {
    m_isnil = true;
    return;
  }
  for ( int i( 0 ); i < l->toList().length(); ++i ) {
    string const & s( l->toList().get( i ).toString().value() );
    if ( s.find( '\t' ) == string::npos ) {
      throw std::string( "No TAB found in ACL entry." );
    }
    m_map[ s.substr( 0, s.find( '\t' ) ) ] = RightList( s.substr( s.find( '\t' ) + 1 ) );
  }
}

void Acl::name( std::string const & dataset ) {
  PList * pl( new Token::PList );
  pl->add( new Token::String( dataset ) );
  m_name = magic_ptr<Token::Token> ( pl );
}

void Acl::name( std::string const & dataset, std::string const & attribute ) {
  PList * pl( new Token::PList );
  pl->add( new Token::String( dataset ) );
  pl->add( new Token::String( attribute ) );
  m_name = magic_ptr<Token::Token>( pl );
}

void Acl::name( std::string const & dataset, std::string const & attribute, std::string const & entry ) {
  PList * pl( new Token::PList );
  pl->add( new Token::String( dataset ) );
  pl->add( new Token::String( attribute ) );
  pl->add( new Token::String( entry ) );
  m_name = magic_ptr<Token::Token>( pl );
}

magic_ptr<Infotrope::Token::Token> Acl::tokens() const {
  if( isNil() ) {
    return magic_ptr<Token::Token>();
  }
  PList * pl( new Token::PList( false ) );
  for ( map<Utils::StringRep::entry_type,RightList>::const_iterator i( m_map.begin() );
	i != m_map.end(); ++i ) {
    string s( ( *i ).first.get() );
    s += "\t";
    s += ( *i ).second.asString();
    pl->add( new Token::String( s ) );
  }
  return magic_ptr<Token::Token> ( pl );
}

RightList Acl::myrights( Utils::StringRep::entry_type const & user, Utils::StringRep::entry_type const & owner ) {
  if( m_rights_cache.find( owner )==m_rights_cache.end() ) {
    m_rights_cache[owner][user]=myrights_calc( user, owner );
  } else if( m_rights_cache[owner].find( user )==m_rights_cache[owner].end() ) {
    m_rights_cache[owner][user]=myrights_calc( user, owner );
  }
  return m_rights_cache[owner][user];
}
RightList Acl::myrights_calc( Utils::StringRep::entry_type const & user, Utils::StringRep::entry_type const & owner ) {
  log( "Myrights Calc setup." );
  if ( user == c_authid_admin ) {
    return RightList( "rwixa" );
  }
  RightList pve;
  RightList nve;
  Utils::StringRep::entry_type nuser( "-"+user.get() );
  Utils::magic_ptr<AuthZ> authz( AuthZ::authz( user ) );
  bool user_owner( user==owner );
  
  log( "Calculating rights." );
  
  for( const_iterator i( m_map.begin() ); i!=m_map.end(); ++i ) {
    log( "Comparing " + user.get() + " to AuthZID " + (*i).first.get() );
    if( user_owner && ( (*i).first == c_authid_owner ) ) {
      log( "Merging owner rights" );
      pve |= (*i).second;
    } else if( user_owner && ( (*i).first == c_authid_not_owner ) ) {
      log( "Merging not-owner rights" );
      nve |= (*i).second;
    } else {
      log( "Trying AuthZ code." );
      if( authz->memberOf( (*i).first ) ) {
	log( "Merging positive rights." );
	pve |= (*i).second;
      } else if( authz->negativeMemberOf( (*i).first ) ) {
	log( "Merging negative rights." );
	nve |= (*i).second;
      }
    }
  }
  
  log( "Okay, done." );
  
  pve -= nve;
  return pve;
}

bool Acl::have_authid( Utils::StringRep::entry_type const & u ) {
  if ( m_map.end() != m_map.find( u ) ) {
    return true;
  }
  return false;
}

void Acl::remove( Utils::StringRep::entry_type const & u ) {
  if ( have_authid( u ) ) {
    m_map.erase( u );
  }
  m_rights_cache.clear();
}

void Acl::grant( Utils::StringRep::entry_type const & u,
                 RightList const & rl ) {
  if( !this ) {
    throw std::runtime_error( "Attempt to grant rights in non-existent Acl." );
  }
  m_isnil = false;
  m_map[ u ] |= rl;
  m_rights_cache.clear();
}

RightList::RightList()
  : m_rights( 0 ) {}



namespace {
  char * g_allrights( "xrwia0" );
}

RightList::RightList( std::string const & s )
  : m_rights( 0 ) {
  for ( char * p( g_allrights ); *p; ++p ) {
    if ( s.find( *p ) != string::npos ) {
      m_rights |= Right::right( *p ).mask();
    }
  }
}

std::string RightList::asString() const {
  string s;
  for ( char * p( g_allrights ); *p; ++p ) {
    if ( m_rights & Right::right( *p ).mask() ) {
      s += *p;
    }
  }
  return s;
}

bool RightList::have_right( Right const & r ) const {
  return m_rights & r.mask();
}

bool RightList::have_right( char ch ) const {
  return have_right( Right::right( ch ) );
}
bool RightList::have_rights( RightList const & r ) const {
  return r.m_rights == ( r.m_rights & m_rights );
}

Right::Right( std::string const & s, unsigned long int m, char ch )
  : m_name( s ), m_mask( m ), m_ch( ch ) {}



Right const & Right::right( char ch ) {
  static map < char, Right * > s_map;
  if ( s_map.end() == s_map.find( ch ) ) {
    s_map[ 'x' ] = new Right( "Search", 1 << 0, 'x' );
    s_map[ 'r' ] = new Right( "Read", 1 << 1, 'r' );
    s_map[ 'w' ] = new Right( "Write", 1 << 2, 'w' );
    s_map[ 'i' ] = new Right( "Insert", 1 << 3, 'i' );
    s_map[ 'a' ] = new Right( "Admin", 1 << 4, 'a' );
    s_map[ '0' ] = new Right( "HintLiteral", 1 << 5, '0' );
  }
  if ( s_map.end() == s_map.find( ch ) ) {
    throw std::string( "No such right " ) + ch;
  }
  return *( s_map[ ch ] );
}

Right const & Right::right( Right::right_name r ) {
  return right( static_cast < char > ( r ) );
}
