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
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/exceptions.hh>
#include <cctype>
#include <algorithm>

#define DEBUG_COMP

#ifdef DEBUG_COMP
#include <infotrope/server/master.hh>
namespace {
  void debug_log( std::string const & s ) {
    Infotrope::Server::Master::master()->log( 1, s );
  }
}
#else
#define debug_log( x ) (void)0
#endif

using namespace Infotrope::Comparators;
using namespace Infotrope;
using namespace std;
using namespace Infotrope::Utils;

// Always used for less<> replacement.
Comparator::compare_result Comparator::order( t_arg const & a,
					      t_arg const & b, bool nil_different ) {
  bool bsafe( true );
  bool asafe( true );
  if ( !a || a->value()->isNil() || a->value()->isList() ) {
    asafe = false;
  }
  if ( !b || b->value()->isNil() || b->value()->isList() ) {
    bsafe = false;
  }
  if ( asafe ) {
    if ( bsafe ) {
      if ( equals( a, b ) ) {
	return Equal;
      }
      if ( comparestrict( a, b ) ) {
	return Lower;
      } else {
	return Higher;
      }
    } else {
      return Lower;
    }
  } else if ( bsafe ) {
    return Higher;
  }
  if ( nil_different ) {
    if ( !a || a->value()->isNil() ) {
      if ( !b || b->value()->isNil() ) {
	return Equal;
      } else {
	return Higher;
      }
    } else {
      if ( !b || b->value()->isNil() ) {
	return Lower;
      } else {
	return Equal;
      }
    }
  }
  return Equal;
}

bool Comparator::comparestrict( t_arg const & ao,
                                t_arg const & bo ) {
  t_tok a( transform( ao ) );
  t_tok b( transform( bo ) );
  if ( b->isNil() ) return true;
  if ( a->isNil() ) return false;
  if ( a->isList() ) return false;
  if ( b->isList() ) return true;
  return comparestrict_internal( a, b );
}

bool Comparator::compare( t_arg const & ao,
                          t_arg const & bo ) {
  t_tok a( transform( ao ) );
  t_tok b( transform( bo ) );
  if ( b->isNil() ) return true;
  if ( a->isNil() ) return false;
  if ( a->isList() ) return false;
  if ( b->isList() ) return true;
  return !comparestrict_internal( b, a );
}

bool Comparator::equals( t_arg const & ao,
                         t_arg const & bo ) {
  //debug_log( "EQUAL: Transform: " + m_name );
  t_tok a( transform( ao ) );
  t_tok b( transform( bo ) );
  //debug_log( "EQUAL : Comparing " + a->asString() + " with " + b->asString() );
  return equals( a, b );
}

bool Comparator::equals( t_tok const & a,
                         t_tok const & b ) {
  
  if ( a->isNil() ) return b->isNil();
  if( b->isNil() ) return a->isNil();
  if ( a->isList() ) {
    for ( int i( 0 ); i < a->toList().length(); ++i ) {
      if ( equals( b, a->toList().ptr( i ) ) ) {
	return true;
      }
    }
    return false;
  } else if ( b->isList() ) {
    return equals( b, a );
  }
  return equals_internal( a, b );
}

bool Comparator::prefix( t_arg const & ao,
			 t_arg const & bo ) {
  return prefix( transform( ao ), transform( bo ) );
}

bool Comparator::prefix( t_tok const & a,
                         t_tok const & b ) {
  if( a->isNil() ) return false;
  if( b->isNil() ) return false;
  if ( b->isList() ) {
    for ( int i( 0 ); i < b->toList().length(); ++i ) {
      if ( prefix( a, b->toList().ptr( i ) ) ) {
	return true;
      }
    }
    return false;
  } else if ( a->isList() ) {
    return false;
  }
  return prefix_internal( a, b );
}

bool Comparator::substring( t_arg const & a,
			    t_arg const & b ) {
  return substring( transform( a ), transform( b ) );
}

bool Comparator::substring( t_tok const & a,
			    t_tok const & b ) {
  if( a->isNil() ) return false;
  if( b->isNil() ) return false;
  if ( b->isList() ) {
    for ( int i( 0 ); i < b->toList().length(); ++i ) {
      if ( substring( a, b->toList().ptr( i ) ) ) {
	return true;
      }
    }
    return false;
  } else if ( a->isList() ) {
    return false;
  }
  return substring_internal( a, b );
}

#include <infotrope/server/master.hh>
#include <infotrope/datastore/constants.hh>

namespace {
  t_arg const * const s_default() {
    static t_arg s( new Data::Attribute( Constants::c_entry_empty, t_tok( new Token::String() ) ) );
    return &s;
  }
}

t_tok const & Comparator::transform( t_arg const & a ) {
  t_arg const * x;
  if( !a ) {
    x = s_default();
  } else {
    x = &a;
  }
  //debug_log( "Transform requested." );
  t_tok const & r( (*x)->value_transform( m_magic_num ) );
  if( !r.ptr() ) {
    //debug_log( "Transform not cached for " + m_name + ", attribute is '" + (*x)->attribute() + "', value is '" + (*x)->value()->asString() + "'" );
    t_tok n( transform_internal( (*x) ) );
    return (*x)->value_transform( m_magic_num, n );
  }
  return r;
}

bool Octet::comparestrict_internal( t_tok const & a,
                                    t_tok const & b ) {
  if ( m_positive ) {
    return ( a->toString().value() < b->toString().value() );
  } else {
    return ( a->toString().value() > b->toString().value() );
  }
}

bool Octet::equals_internal( t_tok const & a,
                             t_tok const & b ) {
  return a->toString().value() == b->toString().value();
}

bool Octet::prefix_internal( t_tok const & a,
			     t_tok const & b ) {
  return b->toString().value().find( a->toString().value() )==0;
}

bool Octet::substring_internal( t_tok const & a,
				t_tok const & b ) {
  return b->toString().value().find( a->toString().value() )!=string::npos;
}

t_tok Octet::transform_internal( t_arg const & a ) {
  if( a->value().ptr() ) {
    return a->value();
  } else {
    return (*s_default())->value();
  }
}

namespace {
  t_tok make_numeric( t_tok const & c ) {
    string const & s( c->toString().value() );
    bool chk( false );
    long unsigned int res( 0 );
    for ( string::const_iterator i( s.begin() );
	  i != s.end(); ++i ) {
      if ( !isdigit( *i ) ) {
	break;
      }
      res *= 10;
      res += ( *i ) - '0';
      chk = true;
    }
    if ( chk ) {
      return t_tok( new Token::Integer( res ) );
    } else {
      return t_tok( new Token::Integer );
    }
  }
}

t_tok AsciiNumeric::transform_internal( t_arg const & a ) {
  if( a->value()->isNil() ) {
    return (*s_default())->value();
  }
  if( a->value()->isList() ) {
    t_tok r( new Token::PList() );
    for( int i(0); i!=a->value()->toList().length(); ++i ) {
      r->toList().add( make_numeric( a->value()->toList().ptr( i ) ) );
    }
    return r;
  }
  return make_numeric( a->value() );
}

bool AsciiNumeric::comparestrict_internal( t_tok const & at,
					   t_tok const & bt ) {
  if ( at->isNil() ) return false;
  if ( bt->isNil() ) return true;
  if ( m_positive ) {
    return ( at->toInteger().value() < bt->toInteger().value() );
  } else {
    return ( at->toInteger().value() > bt->toInteger().value() );
  }
}

bool AsciiNumeric::equals_internal( t_tok const & at,
                                    t_tok const & bt ) {
  if ( at->isNil() ) return false;
  if ( bt->isNil() ) return false;
  return ( at->toInteger().value() == bt->toInteger().value() );
}

t_tok AsciiCasemap::transform_internal( t_arg const & a ) {
  if( a->value()->isNil() ) {
    return (*s_default())->value();
  } else if( a->value()->isList() ) {
    t_tok r( new Token::PList );
    for( int i(0); i!=a->value()->toList().length(); ++i ) {
      r->toList().add( new Token::Atom( a->value()->toList().get(i).toString().value() ) );
    }
    return r;
  }
  return t_tok( new Token::Atom( a->value()->toString().value() ) );
}

bool AsciiCasemap::equals_internal( t_tok const & ai,
                                    t_tok const & bi ) {
  return ai->toAtom().value() == bi->toAtom().value();
}

bool AsciiCasemap::comparestrict_internal( t_tok const & a,
					   t_tok const & b ) {
  if ( m_positive ) {
    return a->toAtom().value() < b->toAtom().value();
  } else {
    return a->toAtom().value() > b->toAtom().value();
  }
}

bool AsciiCasemap::prefix_internal( t_tok const & a,
				    t_tok const & b ) {
  return b->toAtom().value().find( a->toAtom().value() )==0;
}
bool AsciiCasemap::substring_internal( t_tok const & a,
				       t_tok const & b ) {
  return b->toAtom().value().find( a->toAtom().value() )!=Infotrope::Utils::istring::npos;
}
// DEFAULTS

bool Comparator::equals_internal( t_tok const &,
                                  t_tok const & ) {
  throw Data::Exceptions::comparator_error( "Cannot use comparator with EQUALS" );
}
bool Comparator::comparestrict_internal( t_tok const &,
					 t_tok const & ) {
  throw Data::Exceptions::comparator_error( "Cannot use comparator with COMPARESTRICT" );
}
bool Comparator::prefix_internal( t_tok const &,
                                  t_tok const & ) {
  throw Data::Exceptions::comparator_error( "Cannot use comparator with PREFIX" );
}
bool Comparator::substring_internal( t_tok const &,
                                     t_tok const & ) {
  throw Data::Exceptions::comparator_error( "Cannot use comparator with SUBSTRING" );
}

// LOOKUP

#include <map>

magic_ptr<Comparator> const & Comparator::comparator( string const & name ) {
  static std::map< std::string, Utils::magic_ptr<Comparator> > s_map;
  static Threading::Mutex s_lock;
  Threading::Lock l__inst( s_lock );
  string tmp( name );
  if ( tmp[ 0 ] != '-' && tmp[ 0 ] != '+' ) {
    tmp = "+" + tmp;
  }
  if ( s_map.end() == s_map.find( tmp ) ) {
    string n( tmp.substr( 1 ) );
    if ( n == "i;octet" ) {
      s_map[ tmp ] = magic_ptr < Comparator > ( new Octet( tmp[ 0 ] == '+', tmp ) );
    } else if ( n == "i;ascii-casemap" ) {
      s_map[ tmp ] = magic_ptr < Comparator > ( new AsciiCasemap( tmp[ 0 ] == '+', tmp ) );
    } else if ( n == "i;ascii-numeric" ) {
      s_map[ tmp ] = magic_ptr < Comparator > ( new AsciiNumeric( tmp[ 0 ] == '+', tmp ) );
    } else {
      throw Data::Exceptions::comparator_error( "Unknown comparator requested: " + name + " >> " + tmp );
    }
  }
  return s_map[ tmp ];
}
