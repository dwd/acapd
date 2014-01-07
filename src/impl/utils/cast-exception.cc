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
#include <infotrope/utils/cast-exception.hh>
#include <sstream>
#include <cxxabi.h>

using namespace Infotrope::Utils::Exceptions;

cast::cast( std::string const & really,
            std::string const & attempted )
		: m_really( really ),
		m_attempted( attempted ),
m_lookslike( "<unknown>" ) {
	//int i( *(int *)0 );
}



cast::cast( std::string const & really, std::string const & attempted, std::string const & lookslike )
		: m_really( really ), m_attempted( attempted ), m_lookslike( lookslike ) {
	//int i( *(int *)0 );
}



const char * cast::what() const throw() {
	char b[ 1024 ];
	int i( 0 );
	__SIZE_TYPE__ l( 1024 );
	std::stringstream ss;
	ss << "Tried casting \"" << m_lookslike << "\", a ";
	std::string really( abi::__cxa_demangle( m_really.c_str(), b, &l, &i ) );
	i = 0;
	l = 1024;
	std::string attempted( abi::__cxa_demangle( m_attempted.c_str(), b, &l, &i ) );
	//string::size_type i3( really.find_last_of( ":" ) );
	//if( i3!=string::npos ) {
	//  really = really.substr( i3+1 );
	//}
	//string::size_type i2( attempted.find_last_of( ":" ) );
	//if( i2!=string::npos ) {
	//  attempted = attempted.substr( i2+1 );
	//}
	ss << really << " to a " << attempted;
	return ss.str().c_str();
}
