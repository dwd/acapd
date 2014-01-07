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
#ifndef INFOTROPE_SERVER_EXCEPTIONS_HH
#define INFOTROPE_SERVER_EXCEPTIONS_HH

#include <string>
#include <stdexcept>

namespace Infotrope {
  
  namespace Server {
    
    namespace Exceptions {
      class Exception : public std::runtime_error {
      protected:
	Exception( Utils::magic_ptr < Infotrope::Token::PList > const & e, std::string const & s, std::string const & failtok );
	Exception( std::string const & s, std::string const & failtok );
	
	void respcode( Utils::magic_ptr<Infotrope::Token::PList> const & p ) {
	  m_respcode = p;
	}
	void respcode( std::string const & code, std::string const & payload ) {
	  m_respcode = new Infotrope::Token::PList();
	  m_respcode->add( code );
	  m_respcode->add( new Infotrope::Token::String( payload ) );
	}
	void respcode( std::string const & code, std::string const & payload, std::string const & payload2 ) {
	  m_respcode = new Infotrope::Token::PList();
	  m_respcode->add( code );
	  m_respcode->add( new Infotrope::Token::String( payload ) );
	  m_respcode->add( new Infotrope::Token::String( payload2 ) );
	}
	
      public:
	Utils::magic_ptr < Infotrope::Token::PList > const & respcode() const {
	  return m_respcode;
	}
	Infotrope::Token::Atom * failtok() const {
	  return new Infotrope::Token::Atom( m_failtok );
	}
	
	virtual ~Exception() throw() {}
	
	
      private:
	Utils::magic_ptr < Infotrope::Token::PList > m_respcode;
	std::string const m_failtok;
      };
      
      class bad : public Exception {
      public:
	bad( std::exception const & e );
	bad( std::string const & e );
	bad( Utils::magic_ptr < Infotrope::Token::PList > const & e, std::string const & s );
	
	virtual ~bad() throw() {}
      };
      
      class no : public Exception {
      public:
	no( std::string const & e );
	no( Utils::magic_ptr < Infotrope::Token::PList > const & e, std::string const & s );
	
	virtual ~no() throw() {}
      };
    }
  }
}

#endif
