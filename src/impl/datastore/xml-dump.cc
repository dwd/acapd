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
#include <infotrope/datastore/xml-dump.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/server/master.hh>
#include <infotrope/utils/base64.hh>
#include <infotrope/utils/utf8.hh>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <sstream>

using namespace Infotrope;
using namespace Infotrope::Server;
using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope::Utils;
using namespace std;

Dump::Dump() : m_xml() {
  m_xml.exceptions( ios::badbit|ios::failbit|ios::eofbit ); // Generate exceptions.
}

void Dump::open( std::string const & f ) {
  m_xml.open( f.c_str() );
}

std::string Dump::entity_encode( std::string const & si, char c, std::string const & rep ) {
  string::size_type i( 0 );
  string s( si );
  while ( string::npos != ( i=s.find( c, i ) ) ) {
    s.replace( i, 1, rep );
    i += rep.length();
  }
  return s;
}

std::string Dump::dump_value( std::string const & val ) {
  std::string ret( entity_encode( val, '&', "&amp;" ) );
  ret = entity_encode( ret, '\'', "&apos;" );
  ret = entity_encode( ret, '"', "&quot;" );
  ret = entity_encode( ret, '<', "&lt;" );
  ret = entity_encode( ret, '>', "&gt;" );
  return ret;
}

void Dump::dump_value_xml( std::string const & val ) {
  if( Utf8::validate( val, false ) ) {
    m_xml << "<value encoding='binary'>" << dump_value( val ) << "</value>\n";
  } else {
    m_xml << "<value encoding='base64'>" << Base64::encode( val ) << "</value>\n";
  }
}

void Dump::dump_sc( Subcontext const & s ) {
  for ( Subcontext::const_iterator i( s.begin() );
	i != s.end(); ++i ) {
    if( (*i).second != c_entry_empty ) {
      magic_ptr<Entry> const & e( ( *i ).first );
      dump_entry( (*i).second.get(), *e );
    }
  }
}

void Dump::dump_entry( std::string const & n, Entry const & e ) {
  dump_spacer();
  m_xml << "<entry name='" << dump_value( n ) << "'>\n";
  m_spacer++;
  
  for ( Entry::const_iterator ei( e.begin() ); ei != e.end(); ++ei ) {
    // Attribute
    dump_spacer();
    m_xml << "<attribute name='" << dump_value( ( *ei ).first.get() ) << "'>" << endl;
    m_spacer++;
    // Value
    if( !(*ei).second->value()->isNil() ) {
      if ( ( *ei ).second->value()->isList() ) {
	dump_spacer();
	m_xml << "<multivalue>\n";
	m_spacer++;
	for ( int i( 0 ); i < ( *ei ).second->value()->toList().length(); ++i ) {
	  dump_spacer();
	  dump_value_xml( (*ei).second->value()->toList().get(i).toString().value() );
	}
	m_spacer--;
	dump_spacer();
	m_xml << "</multivalue>\n";
      } else {
	dump_spacer();
	dump_value_xml( ( *ei ).second->valuestr() );
      }
    } else if( e.attr_isnil( (*ei).first ) ) {
      dump_spacer();
      m_xml << "<nil/>\n";
    } else {
      dump_spacer();
      m_xml << "<default/>\n";
    }
    // ACL
    dump_spacer();
    m_xml << "<acl>\n";
    m_spacer++;
    if ( !( *ei ).second->acl()->isNil() ) {
      for ( Acl::const_iterator ai( ( *ei ).second->acl()->begin() );
	    ai != ( *ei ).second->acl()->end(); ++ai ) {
	dump_spacer();
	m_xml << "<ace identifier='" << dump_value( ( *ai ).first.get() )
	      << "' rights='" << dump_value( ( *ai ).second.asString() ) << "'/>\n";
      }
    } else if( e.acl_isnil( (*ei).first ) ) {
      dump_spacer();
      m_xml << "<nil/>\n";
    } else if( e.acl_isdefault( (*ei).first ) ) {
      dump_spacer();
      m_xml << "<default/>\n";
    }
    m_spacer--;
    dump_spacer();
    m_xml << "</acl>\n";
    m_spacer--;
    dump_spacer();
    m_xml << "</attribute>\n";
  }
  
  m_spacer--;
  dump_spacer();
  m_xml << "</entry>\n";
}

void Dump::dump_main() {
  Datastore const & ds( Datastore::datastore_read() );
  for ( Datastore::const_iterator d( ds.begin() ); d != ds.end(); ++d ) {
    if ( ds.exists( (*d).first )
	 // && ( Path( (*d).first ).canonicalize()==Path( (*d).first ) // Not needed, we only store canonical paths now.
	 // && (*d).second->exists2( c_entry_empty ) // Datastore does this.
	 // && !( *d ).second->fetch( c_entry_empty )->exists( c_attr_dataset_type ) // isNormal does this.
	 && (*d).second->isNormal()
	 ) {
      dump_spacer();
      m_xml << "<dataset name='" << dump_value( ( *d ).first.get() ) << "'>\n";
      m_spacer++;
      dump_entry( c_entry_empty.get(), *( *d ).second->fetch2( c_entry_empty ) );
      m_spacer--;
      dump_spacer();
      m_xml << "</dataset>\n";
    }
  }
  for ( Datastore::const_iterator d( ds.begin() ); d != ds.end(); ++d ) {
    if ( ds.exists( (*d).first )
	 // && ( Path( (*d).first ).canonicalize()==Path( (*d).first ) // Not needed, we only store canonical paths now.
	 // && (*d).second->exists2( c_entry_empty ) // Datastore does this.
	 // && !( *d ).second->fetch( c_entry_empty )->exists( c_attr_dataset_type ) // isNormal does this.
	 && (*d).second->isNormal()
	 ) {
      dump_spacer();
      m_xml << "<dataset name='" << dump_value( ( *d ).first.get() ) << "'>\n";
      m_spacer++;
      dump_sc( *( *d ).second->requested_subcontext() );
      m_spacer--;
      dump_spacer();
      m_xml << "</dataset>\n";
    }
  }
}

void Dump::dump() {
  m_spacer = 0;
  m_xml << "<acapservers>\n";
  m_spacer++;
  dump_spacer();
  m_xml << "<acap name='localhost'>\n";
  m_spacer++;
  dump_spacer();
  m_xml << "<modify mode='replace'>\n";
  m_spacer++;
  dump_main();
  m_spacer--;
  dump_spacer();
  m_xml << "</modify>\n";
  m_spacer--;
  dump_spacer();
  m_xml << "</acap>\n";
  m_spacer--;
  dump_spacer();
  m_xml << "</acapservers>\n";
  m_xml << flush;
}

void Dump::dump_spacer() {
  for ( int i( 0 ); i < m_spacer; ++i ) {
    m_xml << " ";
  }
}

void Dump::dumper( bool loop, bool trans ) {
  try {
    unsigned long max_files( 10 );
    std::string datadir;
    Datastore const & ds( Datastore::datastore_read() );
    for ( ; ; ) {
      {
	Infotrope::Threading::ReadLock( ds.lock() );
	std::string max_dump_files;
	if ( ds.exists( string( "/vendor.infotrope.acapd/site/" ) ) ) {
	  Utils::magic_ptr < Dataset > dset( ds.dataset( string( "/vendor.infotrope.acapd/site/" ) ) );
	  Utils::magic_ptr < Entry > e( dset->fetch2( "max-dump-files" ) );
	  if ( e && e->exists( "vendor.infotrope.acapd.value" ) ) {
	    max_dump_files = e->attr( "vendor.infotrope.acapd.value" ) ->valuestr();
	  }
	}
	if ( max_dump_files.empty() ) {
	  max_files = 10;
	} else {
	  istringstream ss( max_dump_files );
	  ss >> max_files;
	}
	
	datadir = Master::master()->datadir();
	std::string filename( datadir + ( trans ? "tran-" : "dump-" ) + Modtime().asString() + ".adif" );
	open( filename );
	dump();
	m_xml.close();
      }
      
      {
	DIR * d( opendir( datadir.c_str() ) );
	map<string, string> files;
	map<string, string> tfiles;
	Modtime latest;
	if ( d ) {
	  for ( struct dirent * de( readdir( d ) );
		de; de = readdir( d ) ) {
	    string file( de->d_name );
	    if ( file.substr( 0, 5 ) == "dump-" ) {
	      Modtime m( file.substr( 5, latest.asString().length() ) );
	      if ( m > latest ) latest = m;
	      files[ m.asString() ] = file;
	    }
	    if( file.substr( 0, 5 ) == "tran-" ) {
	      Modtime m( file.substr( 5, latest.asString().length() ) );
	      if( m > latest ) latest = m;
	      tfiles[ m.asString() ] = file;
	    }
	  }
	  closedir( d );
	}
	for ( map < string, string > ::iterator i( files.begin() );
	      i != files.end(); i = files.begin() ) {
	  if ( files.size() < max_files ) {
	    break;
	  }
	  unlink( ( datadir + ( *i ).second ).c_str() );
	  files.erase( i );
	}
	map<string,string>::const_iterator first( files.begin() );
	if( first!=files.end() ) {
	  for( map<string,string>::const_iterator i( tfiles.begin() ); i!=tfiles.end(); ++i ) {
	    if( (*i).first < (*first).first ) {
	      unlink( ( datadir + (*i).second ).c_str() );
	    }
	  }
	}
      }
      if( !loop ) break;
    }
  } catch ( std::string & e ) {
    Master::master()->log( 8, "Dump died with UGLY "+e );
  } catch ( std::exception & e ) {
    Master::master()->log( 8, std::string("Dump died with BAD ")+e.what() );
  } catch ( ... ) {
    Master::master()->log( 8, "Dump died with unknown exception" );
  }
}

Dump::~Dump() {
}
