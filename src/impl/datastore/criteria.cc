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
#include <infotrope/datastore/criteria.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>

using namespace Infotrope::Data::Criteria;
using namespace Infotrope::Constants;
using namespace Infotrope::Data;
using namespace Infotrope::Comparators;
using namespace Infotrope::Utils;
using namespace Infotrope::Token;
using namespace std;
using namespace Infotrope::Utils::StringRep;

namespace {
  t_iter_range range_all( Subcontext const & s ) {
    t_iter_range l;
    l.push_back( make_pair( s.begin(), s.end() ) );
    return l;
  }
  
  // I hoped there'd be a faster way of doing this, but there's no < for
  // t_iter. Bummer, eh?
  t_iter_range range_or( Subcontext const & s, t_iter_range const & left, t_iter_range const & right ) {
    if ( left.empty() ) {
      return right;
    }
    if ( right.empty() ) {
      return left;
    }
    t_iter_range all( range_all( s ) );
    if ( left == all || right == all ) {
      return all;
    }
    
    t_iter_range ret; {
      t_iter_range::const_iterator r( left.begin() );
      t_iter_range::const_iterator l( right.begin() );
      t_iter rb( s.begin() );
      bool inrange( false );
      bool inl( false );
      bool inr( false );
      
      for ( t_iter x( s.begin() ); x != s.end(); ++x ) {
	if ( inr ) {
	  // Look for end of right.
	  if ( x == ( *r ).second ) {
	    ++r;
	    inr = false;
	  }
	} else {
	  // Look for beginning of right.
	  if ( x == ( *r ).first ) {
	    inr = true;
	  }
	}
	
	if ( inl ) {
	  if ( x == ( *l ).second ) {
	    ++l;
	    inl = false;
	  }
	} else {
	  if ( x == ( *l ).first ) {
	    inl = true;
	  }
	}
	
	if ( inrange ) {
	  // I thought I was in the range.
	  if ( !( inl || inr ) ) {
	    // I'm not now, though.
	    inrange = false;
	    ret.push_back( make_pair( rb, x ) );
	  }
	} else {
	  if ( inl || inr ) {
	    inrange = true;
	    rb = x;
	  }
	}
	
      }
      if ( inrange ) {
	ret.push_back( make_pair( rb, s.end() ) );
      }
    }
    return ret;
  }
  
  t_iter_range range_and( Subcontext const & s, t_iter_range const & left, t_iter_range const & right ) {
    t_iter_range all( range_all( s ) );
    if ( left == all ) {
      return right;
    }
    if ( right == all ) {
      return left;
    }
    if ( left.empty() ) {
      return t_iter_range();
    }
    if ( right.empty() ) {
      return t_iter_range();
    }
    t_iter_range ret; {
      t_iter_range::const_iterator r( left.begin() );
      t_iter_range::const_iterator l( right.begin() );
      t_iter rb( s.begin() );
      bool inrange( false );
      bool inl( false );
      bool inr( false );
      
      for ( t_iter x( s.begin() ); x != s.end(); ++x ) {
	if ( inr ) {
	  // Look for end of right.
	  if ( x == ( *r ).second ) {
	    ++r;
	    inr = false;
	  }
	} else {
	  // Look for beginning of right.
	  if ( x == ( *r ).first ) {
	    inr = true;
	  }
	}
	
	if ( inl ) {
	  if ( x == ( *l ).second ) {
	    ++l;
	    inl = false;
	  }
	} else {
	  if ( x == ( *l ).first ) {
	    inl = true;
	  }
	}
	
	if ( inrange ) {
	  // I thought I was in the range.
	  if ( !( inl && inr ) ) {
	    // I'm not now, though.
	    inrange = false;
	    ret.push_back( make_pair( rb, x ) );
	  }
	} else {
	  if ( inl && inr ) {
	    inrange = true;
	    rb = x;
	  }
	}
      }
      if ( inrange ) {
	ret.push_back( make_pair( rb, s.end() ) );
      }
    }
    return ret;
  }
  
  t_iter_range range_invert( Subcontext const & s, t_iter_range const & l ) {
    t_iter_range r;
    t_iter last_end( s.begin() );
    for ( t_iter_range::const_iterator i( l.begin() );
	  i != l.end(); ++i ) {
      if ( ( *i ).first != last_end ) {
	r.push_back( make_pair( last_end, ( *i ).first ) );
      }
      last_end = ( *i ).second;
    }
    if ( s.end() != last_end ) {
      r.push_back( make_pair( last_end, s.end() ) );
    }
    return r;
  }
}

Criterion::Criterion() {}



bool Criterion::perform_range( Subcontext const & ) const {
  return false;
}

t_iter_range Criterion::acceptable( entry_type const & login, Subcontext const & s ) const {
  return acceptable( login, s, range_all( s ) );
}

t_iter_range Criterion::acceptable( entry_type const & login, Subcontext const & s, t_iter_range const & rev ) const {
  t_iter_range good;
  for ( t_iter_range::const_iterator x( rev.begin() );
	x != rev.end(); ++x ) {
    t_iter good_begin( ( *x ).second );
    for ( t_iter i( ( *x ).first ); i != ( *x ).second; ++i ) {
      if ( !acceptable( login, s, i, 0 ) ) {
	if ( good_begin != ( *x ).second ) {
	  good.push_back( make_pair( good_begin, i ) );
	}
	good_begin = ( *x ).second;
      } else {
	if ( good_begin == ( *x ).second ) {
	  good_begin = i;
	}
      }
    }
    if ( good_begin != ( *x ).second ) {
      good.push_back( make_pair( good_begin, ( *x ).second ) );
    }
  }
  return good;
}

magic_ptr<Sort> Criterion::fast_sort() const {
  return magic_ptr<Sort>();
}

All::All()
  : Criterion() {}



bool All::acceptable( entry_type const & login, Subcontext const &, t_iter const &, unsigned long int ) const {
  return true;
}

t_iter_range All::acceptable( entry_type const & login, Subcontext const & s ) const {
  return range_all( s );
}

bool All::perform_range( Subcontext const & ) const {
  return true;
}

BinaryComp::BinaryComp( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool noinherit )
  : Criterion(), m_attr( at ), m_key( new Attribute( c_entry_empty, tok ) ), m_comp( Comparator::Comparator::comparator( comp ) ), m_noinherit( noinherit ), m_search_right( Right::right( 'x' ) ) {
}



bool BinaryComp::acceptable( entry_type const & login, Subcontext const &, t_iter const & i, unsigned long int ) const {
  t_entry const & e( ( *i ).first );
  t_attr tmp( e->attr2( m_attr ) );
  if( tmp ) {
    Utils::magic_ptr<Attribute> a( e->attr2( c_attr_control ) );
    if( !m_last_dset || ( m_last_dset->path().asStringRep()!=a->value_rep() ) ) {
      m_last_dset = Datastore::datastore_read().dataset( a->value_rep() );
    }
    if( m_noinherit ) {
      if( !m_last_dset->myrights( e->attr( c_attr_entry )->value_rep(), m_attr, login ).have_right( m_search_right ) ) {
	tmp.zap();
      }
    } else {
      if( !m_last_dset->myrights( e->attr( c_attr_entry )->value_rep(), m_attr, login, e, tmp ).have_right( m_search_right ) ) {
	tmp.zap();
      }
    }
  }
  return acceptable_int( tmp );
}

Equal::Equal( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool nih )
  : BinaryComp( at, comp, tok, nih ),
    m_octet( comp == "i;octet" || comp == "+i;octet" || comp == "-i;octet" ) {}

bool Equal::acceptable_int( t_attr const & a ) const {
  return m_comp->equals( m_key, a );
}

bool Equal::perform_range( Subcontext const & ) const {
  if ( m_attr == c_attr_entry ) {
    if ( m_key->value()->isNil() || m_octet ) {
      return true;
    }
  }
  return false;
}

t_iter_range Equal::acceptable( entry_type const & login, Subcontext const & s ) const {
  if ( m_attr == c_attr_entry ) {
    if ( m_key->value()->isNil() ) {
      return t_iter_range();   // Nothing, since no "entry" can be NIL.
    }
    else if ( m_octet ) {
      // We can still do things quick if the subcontext wasn't generated with DEPTH, since then the "entry" will be the lookup key.
      // This heavily optimises the case where the client wants a single entry.
      // It's quite rare, but a poorly written ACAP client would otherwise
      // use very heavy resources.
      // The other situation, only ever using ALL, is already optimised.
      if ( ( *s.begin() ).second->find( '/' ) == std::string::npos ) {
	t_iter_range l;
	if( s.exists2( m_key->value()->toString().value() ) ) {
	  t_iter i( s.fetch_iterator( m_key->value()->toString().value() ) );
	  t_iter j( i );
	  l.push_back( make_pair( i, ++j ) );
	}
	return l;
      }
      // But otherwise, we'll have to search through the lot.
    }
    
    
  }
  return Criterion::acceptable( login, s );
}

CompareStrict::CompareStrict( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool nih )
  : BinaryComp( at, comp, tok, nih ) {}



bool CompareStrict::acceptable_int( t_attr const & a ) const {
  return m_comp->comparestrict( m_key, a );
}

Compare::Compare( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool nih )
  : BinaryComp( at, comp, tok, nih ) {}



bool Compare::acceptable_int( t_attr const & a ) const {
  return m_comp->compare( m_key, a );
}


Prefix::Prefix( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool nih )
  : BinaryComp( at, comp, tok, nih ) {}



bool Prefix::acceptable_int( t_attr const & a ) const {
  return m_comp->prefix( m_key, a );
}


Substring::Substring( entry_type const & at, string const & comp, magic_ptr<Token::Token> const & tok, bool nih )
  : BinaryComp( at, comp, tok, nih ) {}



bool Substring::acceptable_int( t_attr const & a ) const {
  return m_comp->substring( m_key, a );
}




Not::Not( magic_ptr < Criterion > const & c )
  : Criterion(), m_crit( c ) {}



bool Not::acceptable( entry_type const & login, Subcontext const & s, t_iter const & e, unsigned long int i ) const {
  return !m_crit->acceptable( login, s, e, i );
}

t_iter_range Not::acceptable( entry_type const & login, Subcontext const & s ) const {
  return range_invert( s, m_crit->acceptable( login, s ) );
}

t_iter_range Not::acceptable( entry_type const & login, Subcontext const & s, t_iter_range const & r ) const {
  return range_invert( s, m_crit->acceptable( login, s, r ) );
}

bool Not::perform_range( Subcontext const & s ) const {
  return m_crit->perform_range( s );
}

And::And( magic_ptr < Criterion > const & c1, magic_ptr < Criterion > const & c2 )
  : Criterion(), m_crit1( c1 ), m_crit2( c2 ) {}



bool And::acceptable( entry_type const & login, Subcontext const & s, t_iter const & e, unsigned long int i ) const {
  return m_crit1->acceptable( login, s, e, i ) && m_crit2->acceptable( login, s, e, i );
}

Or::Or( magic_ptr < Criterion > const & c1, magic_ptr < Criterion > const & c2 )
  : Criterion(), m_crit1( c1 ), m_crit2( c2 ) {}



bool Or::acceptable( entry_type const & login, Subcontext const & s, t_iter const & e, unsigned long int i ) const {
  return m_crit1->acceptable( login, s, e, i ) || m_crit2->acceptable( login, s, e, i );
}

/*
  Say left is 0-3,7-8
  Right is 2-5,9-10
  
  Final is 0-5,7-8,9-10
  
  Consider:
  0-3  2-5 OVERLAP
  Overlap, because beginning of one range is between the begin and end of the other.
  0-5  2-3 INSIDE
  Inside, because the beginning of one range is after the beginning of the other,
  and the end of the same range is before the end of the other.
  
  0-3  5-7 NOTHING
  0-3  3-6 APPEND
*/

namespace {
  t_iter_range restrict_range_or( entry_type const & login, Subcontext const & s, t_iter_range const & range, magic_ptr < Criterion > const & crit ) {
    t_iter_range all( range_all( s ) );
    if ( all == range ) {
      return all;
    }
    return range_or( s, crit->acceptable( login, s, range_invert( s, range ) ), range );
  }
  t_iter_range restrict_range_and( entry_type const & login, Subcontext const & s, t_iter_range const & range, magic_ptr < Criterion > const & crit ) {
    return crit->acceptable( login, s, range );
  }
}


t_iter_range Or::acceptable( entry_type const & login, Subcontext const & s ) const {
  // Interesting... If one of the options is range capable, then it's worth
  // restricting our comparison to just this.
  // If both are range capable, then we can add the ranges together.
  if ( m_crit1->perform_range( s ) ) {
    return restrict_range_or( login, s, m_crit1->acceptable( login, s ),
			      m_crit2 );
  } else {
    return restrict_range_or( login, s, m_crit2->acceptable( login, s ),
			      m_crit1 );
  }
}

bool Or::perform_range( Subcontext const & s ) const {
  return true;
}

t_iter_range And::acceptable( entry_type const & login, Subcontext const & s ) const {
  // Interesting... If one of the options is range capable, then it's worth
  // restricting our comparison to just this.
  // If both are range capable, then we can add the ranges together.
  if ( m_crit1->perform_range( s ) ) {
    return restrict_range_and( login, s, m_crit1->acceptable( login, s ),
			       m_crit2 );
  } else {
    return restrict_range_and( login, s, m_crit2->acceptable( login, s ),
			       m_crit1 );
  }
}

bool And::perform_range( Subcontext const & s ) const {
  return true;
}

Range::Range( long unsigned int from, long unsigned int to )
  : m_from( from ), m_to( to ) {}



bool Range::acceptable( entry_type const & login, Subcontext const & s, t_iter const & x, unsigned long int xx ) const {
  if( xx>0 ) {
    return ( xx >= m_from ) && ( xx <= m_to );
  }
  unsigned long int count( 1 );
  t_iter i( x );
  while ( i != s.begin() ) {
    --i;
    ++count;
  }
  if ( ( count >= m_from ) && ( count <= m_to ) ) {
    return true;
  }
  return false;
}

t_iter_range Range::acceptable( entry_type const & login, Subcontext const & s, t_iter_range const & r ) const {
  unsigned long int count( 0 );
  t_iter_range::const_iterator rr( r.begin() );
  t_iter_range ret;
  t_iter good_begin( s.end() );
  t_iter good_end( ( *rr ).second );
  bool state( false );
  
  for ( t_iter i( s.begin() ); i != s.end(); ++i ) {
    bool init( false );
    bool inenum( false );
    ++count;
    if ( i == ( *rr ).second ) {
      ++rr;
      if ( rr == r.end() ) {
	break;
      }
      good_end = ( *rr ).second;
    } else if ( i == ( *rr ).first ) {
      init = true;
    }
    
    if ( count >= m_from ) {
      if ( count <= m_to ) {
	inenum = true;
      }
    }
    
    if ( state ) {
      if ( !init || !inenum ) {
	ret.push_back( make_pair( good_begin, i ) );
	state = false;
      }
    } else {
      if ( init && inenum ) {
	good_begin = i;
	state = true;
      }
    }
  }
  if ( state == true ) {
    if ( rr != r.end() ) {
      good_end = s.end();
    }
    ret.push_back( make_pair( good_begin, good_end ) );
  }
  return ret;
}

t_iter_range Range::acceptable( entry_type const & login, Subcontext const & s ) const {
  unsigned long int count( 0 );
  t_iter_range ret;
  t_iter good_begin( s.end() );
  bool state( false );
  
  for ( t_iter i( s.begin() ); i != s.end(); ++i ) {
    bool inenum( false );
    ++count;
    
    if ( count >= m_from ) {
      if ( count <= m_to ) {
	inenum = true;
      }
    }
    
    if ( state ) {
      if ( !inenum ) {
	ret.push_back( make_pair( good_begin, i ) );
	state = false;
      }
    } else {
      if ( inenum ) {
	good_begin = i;
	state = true;
      }
    }
  }
  if ( state ) {
    ret.push_back( make_pair( good_begin, s.end() ) );
  }
  return ret;
}

bool Range::perform_range( Subcontext const & s ) const {
  return true;
}
