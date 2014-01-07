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
#include <infotrope/datastore/path.hh>
#include <infotrope/datastore/exceptions.hh>

//#include <iostream>

using namespace Infotrope::Data;
using namespace Infotrope::Data::Exceptions;
using namespace std;
using namespace Infotrope::Utils::StringRep;

namespace {
  entry_type const c_byowner( "byowner" );
  entry_type const c_user( "user" );
  entry_type const c_group( "group" );
  entry_type const c_host( "host" );
  entry_type const c_site( "site" );
  entry_type const c_tilde( "~" );
}

Path::Path( const char * s )
  : m_string( s ), m_list(), m_mybit(), m_have_user( false ), m_user(), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  decompose();
  list<entry_type>::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  analyze();
}

Path::Path( std::string const & s )
  : m_string( s ), m_list(), m_mybit(), m_have_user( false ), m_user(), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  decompose();
  list<entry_type>::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  analyze();
}

Path::Path( entry_type const & s )
  : m_string( s ), m_list(), m_mybit(), m_have_user( false ), m_user(), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  decompose();
  list<entry_type>::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  analyze();
}

Path::Path( list<entry_type> const & l )
  : m_string(), m_list( l ), m_mybit(), m_have_user( false ), m_user(), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  list < entry_type > ::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  compose();
  analyze();
}

Path::Path( entry_type const & s, entry_type const & user )
  : m_string( s ), m_list(), m_mybit(), m_have_user( false ), m_user( user ), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  decompose();
  list<entry_type>::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  compose();
  analyze();
}

Path::Path( list<entry_type> const & l, entry_type const & user )
  : m_string(), m_list( l ), m_mybit(), m_have_user( false ), m_user( user ), m_have_owner( false ), m_owner(), m_length(0), m_netloc() {
  compose();
  decompose();
  list<entry_type>::const_iterator i( m_list.end() );
  if ( !m_list.empty() ) {
    --i;
    m_mybit = *i;
  }
  analyze();
}

void Path::analyze() {
  list<entry_type>::const_iterator i( m_list.begin() );
  if ( i == m_list.end() ) {
    // Top.
    m_translatable = false;
    m_view = ByClass;
    return ;
  }
  if ( ( *i ) == c_byowner ) {
    m_view = ByOwner;
  } else {
    m_view = ByClass;
  }
  ++i;
  if ( i == m_list.end() ) {
    // Only class or namespace bit.
    m_translatable = false;
    return ;
  }
  if ( ( *i ) == c_tilde ) {
    m_have_user = true;
    ++i;
  } else if ( ( *i ) == c_site ) {
    ++i;
  } else if ( ( *i ) == c_user ) {
    ++i;
    if ( i == m_list.end() ) {
      // Only half the owner part.
      m_translatable = false;
      return ;
    }
    m_owner = *i;
    //std::cout << "Found owner of " << m_owner << "\n";
    ++i;
    m_have_owner = true;
  } else if ( ( *i ) == c_group || ( *i ) == c_host ) {
    ++i;
    if ( i == m_list.end() ) {
      // Only half the owner part.
      m_translatable = false;
      return ;
    }
    ++i;
  } else {
    compose();
    // Weird bit. Maybe extension that we don't know about.
    throw invalid_path( m_string.get(), "Unknown user prefix '" + ( *i ).get() + "' in path." );
  }
  if ( m_view != ByOwner ) {
    // Okay, we have /dataset-class/owner-part/
    m_translatable = true;
    return ;
  }
  if ( i == m_list.end() ) {
    // Bad, we've got /byowner/owner-part/ - no dataset-class
    m_translatable = false;
    return ;
  }
  m_translatable = true;
}

void Path::decompose() {
  if( m_string->find( "acap://" )==0 ) {
    m_string = m_string->substr( 7 );
    m_netloc = m_string->substr( 0, m_string->find( "/" ) );
    m_string = "/" + m_string->substr( m_netloc.length() );
  } else if( m_string->find( "//" )==0 ) {
    m_string = m_string->substr( 2 );
    m_netloc = m_string->substr( 0, m_string->find( "/" ) );
    m_string = "/" + m_string->substr( m_netloc.length() );
  }
  if ( m_string.get()[ 0 ] != '/' ) {
    throw invalid_path( m_string.get(), "No '/' at start" );
  }
  string working( m_string->substr( 1 ) );
  while ( working.length() ) {
    string::size_type i( working.find( '/' ) );
    string tmp;
    if ( i == string::npos ) {
      tmp = working;
    } else {
      tmp = working.substr( 0, i );
    }
    if ( c_tilde == tmp && m_user.ptr() ) {
      m_list.push_back( c_user );
      m_list.push_back( m_user );
    } else {
      m_list.push_back( tmp );
    }
    if ( i == string::npos ) {
      m_string = m_string.get() + "/";
      break;
    }
    working = working.substr( i + 1 );
  }
}

void Path::compose() {
  std::string m( "/" );
  for ( list < entry_type > ::const_iterator i( m_list.begin() );
	i != m_list.end(); ++i ) {
    if ( ( *i ) == c_tilde && m_user.ptr() ) {
      m += "user/";
      m += m_user.get();
    } else {
      m += ( *i ).get();
    }
    m += "/";
  }
  m_string = Utils::StringRep::entry_type( m );
}

Path Path::toByclass() const {
  if ( m_view == ByClass ) {
    return * this;
  }
  list < entry_type > r;
  list < entry_type > ::const_iterator i( m_list.begin() );
  if ( ( *i ) != c_byowner ) {
    throw invalid_path( m_string.get(), "Translation to byclass view requested from byclass namespace." );
  }
  list < entry_type > tmp( m_list );
  tmp.erase( tmp.begin() );
  list < entry_type > owner_part;
  if ( tmp.empty() ) {
    throw invalid_path( m_string.get(), "Path too short for translation [BYO1]." );
  }
  owner_part.push_back( *( tmp.begin() ) );
  tmp.erase( tmp.begin() );
  if ( *( owner_part.begin() ) != c_site && *( owner_part.begin() ) != c_tilde ) {
    if ( tmp.empty() ) {
      throw invalid_path( m_string.get(), "Path too short for translation [BYO2]." );
    }
    owner_part.push_back( *( tmp.begin() ) );
    tmp.erase( tmp.begin() );
  }
  if ( tmp.empty() ) {
    throw invalid_path( m_string.get(), "Path too short for translation [BYO3]." );
  }
  entry_type class_part( *( tmp.begin() ) );
  tmp.erase( tmp.begin() );
  r.push_back( class_part );
  for ( list<entry_type>::const_iterator i( owner_part.begin() );
	i != owner_part.end(); ++i ) {
    r.push_back( *i );
  }
  for ( list<entry_type>::const_iterator i( tmp.begin() );
	i != tmp.end(); ++i ) {
    r.push_back( *i );
  }
  return r;
}

Path Path::toByowner() const {
  if ( m_view == ByOwner ) {
    return * this;
  }
  list<entry_type> r;
  list<entry_type>::const_iterator i( m_list.begin() );
  if ( ( *i ) == c_byowner ) {
    throw invalid_path( m_string.get(), "Translation to byowner view requested from byowner namespace." );
  }
  list<entry_type> tmp( m_list );
  entry_type class_part( *( tmp.begin() ) );
  tmp.erase( tmp.begin() );
  list<entry_type> owner_part;
  if ( tmp.empty() ) {
    throw invalid_path( m_string.get(), "Path too short for translation. [BYC1]" );
  }
  owner_part.push_back( *( tmp.begin() ) );
  tmp.erase( tmp.begin() );
  if ( *( owner_part.begin() ) != c_site && *( owner_part.begin() ) != c_tilde ) {
    if ( tmp.empty() ) {
      throw invalid_path( m_string.get(), "Path too short for translation. [BYC2]" );
    }
    owner_part.push_back( *( tmp.begin() ) );
    tmp.erase( tmp.begin() );
  }
  r.push_back( c_byowner );
  for ( list<entry_type>::const_iterator i( owner_part.begin() );
	i != owner_part.end(); ++i ) {
    r.push_back( *i );
  }
  r.push_back( class_part );
  for ( list<entry_type>::const_iterator i( tmp.begin() );
	i != tmp.end(); ++i ) {
    r.push_back( *i );
  }
  return r;
}

Path Path::parent() const {
  if ( m_list.empty() ) {
    throw invalid_path( m_string.get(), "Parent requested on top dataset." );
  }
  list<entry_type> tmp( m_list );
  list<entry_type>::iterator i( tmp.end() );
  --i;
  tmp.erase( i );
  return tmp;
}

Path Path::operator+( std::string const & s ) const {
  if( s.find( "/" )==0 || s.find( "acap://" )==0 ) {
    return Path( s );
  } else if( s=="." ) {
    throw invalid_path( ".", "Cannot find relative of 'here' to " + m_string.get() );
  } else {
    list<entry_type> tmp( m_list );
    Path ptmp( "/"+s ); // Fudge it a little.
    entry_type dotdot( ".." );
    for( list<entry_type>::const_iterator i( ptmp.m_list.begin() ); i!=ptmp.m_list.end(); ++i ) {
      if( (*i)==dotdot ) {
	tmp.pop_back();
      } else {
	tmp.push_back( *i );
      }
    }
    return Path( tmp );
  }
}  

void Path::validate() const {
  if ( m_have_user ) throw invalid_path( m_string.get(), "User part present but need valid path. We are DOOOOOOOOOOMED, laddie! DOOOOOOOOOOOOOOOMED!!!" );
  if( isRemote() ) throw invalid_path( m_string.get(), "Whoops, remote path for local use. Bad move." );
}
