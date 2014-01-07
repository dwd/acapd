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
#ifndef INFOTROPE_DATASTORE_EXCEPTIONS_HH
#define INFOTROPE_DATASTORE_EXCEPTIONS_HH

#include <stdexcept>
#include <string>
#include <infotrope/datastore/acl.hh>
#include <infotrope/server/exceptions.hh>

namespace Infotrope {
  
  namespace Data {
    
    namespace Exceptions {
      
      /* class Exception : public std::runtime_error {
      public:
	explicit Exception( std::string const & );
	virtual ~Exception() throw() {}
	
	
	}; */
      
      // Attempt to reference a dataset which does not exist.
      // Generates NOEXIST for STORE .. NOCREATE and SEARCH.
      class no_dataset : public Server::Exceptions::no {
      public:
	explicit no_dataset( std::string const & );
	virtual ~no_dataset() throw() {}
	
	std::string const & dataset() const {
	  return m_dataset;
	}
	
      private:
	std::string m_dataset;
      };
      
      // Path supplied is invalid syntax.
      // Possibly invalid because of an unknown owner part.
      // Generates BAD.
      class invalid_path : public Server::Exceptions::bad {
      public:
	explicit invalid_path( std::string const &, std::string const & );
	virtual ~invalid_path() throw() {}
	
	std::string const & path() const {
	  return m_path;
	}
	
      private:
	std::string m_path;
      };
      
      // No permission. Generates NO - I haven't put in the extra code
      // required to support figuring out if I can issue PERMISSION yet.
      class no_permission : public Server::Exceptions::no {
      public:
	explicit no_permission( Acl const &, std::string const & path, Right const & r, std::string const & message );
	virtual ~no_permission() throw() {}
	
	std::string const & path() const {
	  return m_path;
	}
	
	Utils::magic_ptr<Token::Token> const & acl_name() const {
	  return m_acl_name;
	}
	
      private:
	std::string m_path;
	Utils::magic_ptr<Token::Token> m_acl_name;
      };
      
      // Not used.
      class modified : public Server::Exceptions::no {
      public:
	explicit modified( std::string const & entrypath );
	virtual ~modified() throw() {}
      };
      
      // Abstract.
      class invalid : public Server::Exceptions::Exception {
      protected:
	explicit invalid( std::string const & failtok, std::string const & what_invalid, std::string const & invalid_stuff );
      public:
	virtual ~invalid() throw() {}
      };
      
      // General BAD, used for attributes and entry paths.
      class invalid_attr : public invalid {
      public:
	explicit invalid_attr( std::string const & invalid_stuff );
	virtual ~invalid_attr() throw() {}
      };
      
      // Supplied data invalid.
      // Generates NO (INVALID)
      class invalid_data : public invalid {
      public:
	explicit invalid_data( std::string const & path, std::string const & attr, std::string const & invalid_stuff );
	explicit invalid_data( std::string const & failtok, std::string const & path, std::string const & attr, std::string const & invalid_stuff );
	virtual ~invalid_data() throw() {}
	
	std::string const & attr() const {
	  return m_attr;
	}
	std::string const & path() const {
	  return m_path;
	}
      private:
	std::string const m_attr;
	std::string const m_path;
      };
      
      // Supplied entry value invalid.
      // Generates BAD INVALID.
      class invalid_data_sys : public invalid_data {
      public:
	explicit invalid_data_sys( std::string const &, std::string const &, std::string const & );
	virtual ~invalid_data_sys() throw() {}
      };
      
      // Generic BAD.
      class proto_format_error : public Server::Exceptions::bad {
      public:
	explicit proto_format_error( std::string const & );
	virtual ~proto_format_error() throw() {}
      };
      
      // Comparator error.
      class comparator_error : public Server::Exceptions::bad {
      public:
	explicit comparator_error( std::string const & );
	virtual ~comparator_error() throw() {}
      };
    }
    
  }
  
}

#endif
