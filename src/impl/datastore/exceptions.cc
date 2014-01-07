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
#include <infotrope/datastore/exceptions.hh>

using namespace Infotrope::Server::Exceptions;

namespace Infotrope {
  namespace Data {
    namespace Exceptions {
      // Exception::Exception( std::string const & s )
      // : std::runtime_error( s ) {}
      
      
      no_dataset::no_dataset( std::string const & s )
	: no::no( s + " does not exist." ), m_dataset( s ) {
	Utils::magic_ptr<Token::PList> pl( new Token::PList );
	pl->add( "NOEXIST" );
	pl->add( new Token::String( s ) );
	respcode( pl );
      }
      
      
      invalid_path::invalid_path( std::string const & p, std::string const & s )
	: bad::bad( p + " : " + s ), m_path( p ) {}
      
      
      no_permission::no_permission( Acl const & a, std::string const & path, Right const & r, std::string const & message )
	: no::no( std::string( "Do not have right " ) + r.ch() + " on " + path + ": " + message ), m_path( path ), m_acl_name( a.name() ) {
      }
      
      
      modified::modified( std::string const & entrypath )
	: no::no( "Entry has been modified and UNCHANGEDSINCE given" ) {
	respcode( "MODIFIED", entrypath );
      }
      
      invalid::invalid( std::string const & failtok, std::string const & what, std::string const & how )
	: Exception::Exception( what + " was invalid: " + how, failtok ) {
      }
      
      invalid_attr::invalid_attr( std::string const & how )
	: invalid::invalid( "BAD", "Attribute name", how ) {
      }
      
      invalid_data::invalid_data( std::string const & path, std::string const & attr, std::string const & how )
	: invalid::invalid( "NO", "Attribute value", how ), m_attr( attr ), m_path( path ) {
	respcode( "INVALID", path, attr );
      }
      invalid_data::invalid_data( std::string const & failtok, std::string const & path, std::string const & attr, std::string const & how )
	: invalid::invalid( failtok, "Attribute value", how ), m_attr( attr ), m_path( path ) {
	respcode( "INVALID", path, attr );
      }
      
      invalid_data_sys::invalid_data_sys( std::string const & path, std::string const & attr, std::string const & how )
	: invalid_data::invalid_data( "BAD", path, attr, how ) {
      }
      
      proto_format_error::proto_format_error( std::string const & what )
	: bad::bad( "Protocol format error: " + what ) {
      }
      
      comparator_error::comparator_error( std::string const & what )
	: bad::bad( "Comparator Error: " + what ) {
      }
    }
  }
}
