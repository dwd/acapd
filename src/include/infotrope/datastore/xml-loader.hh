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
#ifndef INFOTROPE_DATASTORE_LOADER_HH
#define INFOTROPE_DATASTORE_LOADER_HH

#include <infotrope/utils/xml-parser.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/subcontext.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/attribute.hh>
#include <infotrope/datastore/transaction.hh>

namespace Infotrope {
  namespace Data {
    
    class Loader : public Infotrope::XML::Parser {
    public:
      Loader();
      
      bool sane() const {
	return m_sane;
      }
      
    protected:
      void start( std::string const & t, XML::t_attrs_p const & a );
      void end( std::string const & );
      void text( std::string const & txt );
      
      void set_attribute( std::string const & );
      
    private:
      Infotrope::Utils::magic_ptr < Dataset > m_dataset;
      Utils::magic_ptr < Entry > m_entry;
      Utils::magic_ptr < Attribute > m_attribute;
      Utils::magic_ptr<Token::Token> m_value;
      Utils::magic_ptr < Acl > m_acl;
      Datastore & m_ds;
      std::string m_mode;
      Transaction m_trans;
      bool m_sane;
      bool m_attr_nil;
      bool m_attr_default;
      bool m_acl_nil;
      bool m_acl_default;
      bool m_inh_reset;
    };
  }
}

#endif
