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
 *            acap-attribute.cc
 *
 *  Thu Feb 13 12:50:07 2003
 *  Copyright  2003  Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/
#include <infotrope/datastore/attribute.hh>
#include <sstream>

using namespace Infotrope::Data;
using namespace Infotrope::Utils;
using namespace Infotrope;

Attribute::Attribute( Attribute::t_name const & n )
  : m_value( magic_ptr<Token::Token>( new Token::String ) ), m_attribute( n ), m_acl(), m_origin("/"), m_transform( 4 ) {}



Attribute::Attribute( Attribute::t_name const & n, std::string const & v )
  : m_value( magic_ptr<Token::Token>( new Token::String( v ) ) ), m_attribute( n ), m_acl(), m_origin("/"), m_transform( 4 ) {
}

Attribute::Attribute( Attribute::t_name const & n, Attribute::t_ptr const & v )
  : m_value( v ), m_attribute( n ), m_origin("/"), m_transform( 4 ) {
}

Attribute::Attribute( Attribute::t_name const & n, Utils::magic_ptr<Acl> const & av ) : m_value( av->tokens() ), m_value_acl( av ), m_attribute( n ), m_origin("/"), m_transform( 4 ) {
}

void Attribute::size_calc() {
  if( m_value->isNil() ) {
    m_size = magic_ptr<Token::Token>( new Token::Integer() );
  } else if ( m_value->isList() ) {
    Token::PList * l( new Token::PList );
    for ( int i( 0 ); i < m_value->toList().length(); ++i ) {
      l->add( new Token::Integer( m_value->toList().get( i ).toString().value().length() ) );
    }
    m_size = magic_ptr<Token::Token>( l );
  } else {
    m_size = magic_ptr<Token::Token>( new Token::Integer( m_value->toString().value().length() ) );
  }
}

void Attribute::add( magic_ptr<Token::String> const & s ) {
  if ( m_value->isList() ) {
#ifdef _REENTRANT
    Threading::Lock l__inst(m_lock);
#endif
    m_value->toList().add( magic_cast < Token::Token > ( s ) );
    m_transform.clear();
  } else {
    throw std::string( "Attempt to add value to non-multivalue" );
  }
}

magic_ptr<Attribute> Attribute::clone() const {
  magic_ptr < Attribute > a;
  if( m_value_acl.ptr() ) {
    a = new Attribute( m_attribute, m_value_acl );
  } else if( m_value ) {
    a = new Attribute( m_attribute );
    if( m_value->isNil() ) {
      a->m_value = new Token::String;
    } else if ( m_value->isList() ) {
      a->m_value = new Token::PList( false );
      for ( int i( 0 ); i < m_value->toList().length(); ++i ) {
	a->m_value->toList().add( m_value->toList().ptr( i ) );
      }
    } else {
      a->m_value = new Token::String( m_value->toString().value() );
    }
  }
  a->m_acl = m_acl;
  a->m_active_acl = m_active_acl;
  return a;
}
