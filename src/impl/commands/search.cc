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
#include <infotrope/server/command.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/criteria.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/server/return.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/server/exceptions.hh>
#include <infotrope/server/context.hh>
#include <infotrope/server/master.hh>

#include <sys/time.h>
#include <time.h>
#include <sstream>

using namespace Infotrope;
using namespace Infotrope::Data;
using namespace Infotrope::Utils;
using namespace Infotrope::Constants;

namespace {
  
  static inline void log( int p, std::string const & msg ) {
    // Server::Master::master()->log( p, msg );
  }

  class Search : public Server::Command {
  private:
    unsigned int m_depth;   // 1
    bool m_hard_limit_spec;   // FALSE
    unsigned int m_hard_limit;   // UNSPEC
    bool m_limit_spec;   // FALSE
    bool m_limit_exceeded;   // FALSE
    unsigned int m_limit_limit;   // UNSPEC
    unsigned int m_limit_maxret;   // UNSPEC
    bool m_make_context;   // FALSE
    bool m_context_enumerate;   // FALSE
    bool m_context_notify;   // FALSE
    bool m_context_persist; // FALSE
    std::string m_context;   // UNSPEC
    bool m_noinherit;   // FALSE
    Utils::magic_ptr < Server::Return > m_return ;   // NOEXIST
    Utils::magic_ptr < Token::Token > m_sort;
    std::string m_base_search;   // NODEF
    Utils::magic_ptr < Subcontext > m_subcontext;   // NODEF
    Utils::magic_ptr < Subcontext > m_sdset_subcontext;   // NODEF
    bool m_attribute_star;   // FALSE
    bool m_attribute_multiple;   // FALSE
    Utils::magic_ptr < Server::Context > m_source_context;   // NONE
    Server::Context::t_dset_list m_used_datasets;   // EMPTY
    std::vector<Path> m_path_stack; // EMPTY, TRANSIENT
    bool m_follow_local; // FALSE.
    Right const & m_read_right;
    
  public:
    Search( Server::Worker & w )
      : Server::Command( false, w ), m_depth( 1 ),
	m_hard_limit_spec( false ), m_hard_limit( 0 ), m_limit_spec( false ),
	m_limit_exceeded( false ),
	m_limit_limit( 0 ), m_limit_maxret( 0 ), m_make_context( false ),
	m_context_enumerate( false ), m_context_notify( false ), m_context_persist( false ), m_context(),
	m_noinherit( false ), m_return(), m_sort(), m_base_search(), m_sdset_subcontext( new Subcontext ),
	m_attribute_star( false ), m_attribute_multiple( false ),
	m_source_context(), m_used_datasets(), m_path_stack(), m_follow_local( false ), m_read_right( Right::right( 'r' ) ) {
    }
    
    void internal_parse( bool complete ) {
      if( m_toks->length() > 2 ) {
	if( m_toks->get(2).isString() ) {
	  if( m_toks->get(2).toString().value()[0]=='/' ) {
	    Path p( m_toks->get(2).toString().value() );
	  }
	} else if( !m_toks->get(2).isLiteralMarker() ) {
	  throw std::runtime_error( "Expecting a string for base search, either path or context." );
	}
      }
    }
    
    void build_subcontext( Utils::magic_ptr < Criteria::Criterion > const & crit ) {
      if ( m_sort ) {
	m_subcontext = Infotrope::Utils::magic_ptr < Subcontext > ( new Subcontext( m_sort->toList() ) );
      } else {
	m_subcontext = Infotrope::Utils::magic_ptr < Subcontext > ( new Subcontext );
      }
      if ( m_base_search.empty() ) { // An empty string is entirely valid, and a context.
	build_subcontext_context( crit, m_base_search );
      } else if ( m_base_search[ 0 ] == '/' ) {
	log( 1, "Checking base dataset: '" + m_base_search + "' - '" + m_worker.login() + "'" );
	Path real( m_base_search, m_worker.login() );
	Utils::magic_ptr<Token::PList> pl( new Token::PList( false ) );
	pl->add( new Token::Atom( "NOEXIST" ) );
	pl->add( new Token::String( real.asString() ) );
	log( 1, "Rights check for " + real.asString() );
	if( !Datastore::datastore_read().dataset( real )->myrights( c_entry_empty, c_attr_entry, m_worker.login() ).have_right( m_read_right ) ) {
	  throw Server::Exceptions::no( pl, real.asString() + " does not exist." );
	} else {
	  if( m_noinherit ) {
	    if( !Datastore::datastore_read().dataset( real )->isNormal() ) {
	      throw Server::Exceptions::no( pl, real.asString() + " does not exist." );
	    }
	  }
	}
	log( 1, "Building subcontext." );
	log( 1, "Path 1 is " + Path( m_base_search, m_worker.login() ).canonicalize().asString() );
	log( 1, "Path 2 is " + Path( m_base_search ).asString() );
	build_subcontext_dataset( crit, Path( m_base_search, m_worker.login() ).canonicalize(), Path( m_base_search, m_worker.login() ).canonicalize(), Path( m_base_search ), 1 );
      } else {
	build_subcontext_context( crit, m_base_search );
      }
    }
    
    void build_subcontext_context( Utils::magic_ptr<Criteria::Criterion> const & crit, std::string const & cxt ) {
      m_source_context = m_worker.context( cxt );
      Infotrope::Threading::ReadLock l__inst( m_source_context->cxt_lock() );
      Utils::magic_ptr<Subcontext> s( m_source_context->subcontext_pure() );
      unsigned long int count( 1 );
      for ( Subcontext::const_iterator i( s->begin() );
	    i != s->end(); ++i ) {
	if( !crit || crit->acceptable( m_worker.login(), *s, i, count ) ) {
	  m_subcontext->add_notrans( ( *i ).second, ( *i ).first );
	}
	++count;
      }
    }
    
    void build_subcontext_dataset( Utils::magic_ptr<Criteria::Criterion> const & crit, Path const & path, Path const & logical_path, Path const & display_path, unsigned int cdepth ) {
      log( 1, "Checking dataset " + path.asString() );
      if ( m_depth != 0 ) {
	if ( m_depth < cdepth ) {
	  return ;
	}
      }
      Datastore const & ds( Datastore::datastore_read() );
      {
	magic_ptr<Dataset> dset( ds.dataset( path ) );
	for( std::vector<Path>::const_iterator i( m_path_stack.begin() );
	     i!=m_path_stack.end(); ++i ) {
	  if( (*i)==path ) {
	    Utils::magic_ptr<Token::List> l( new Token::List );
	    l->add( "*" );
	    l->add( "NO" );
	    l->add( new Token::QuotedString( "Recursion loop detected, ignoring repeated dataset: " + path.asString() ) );
	    m_worker.send( l );
	    log( 1, "Recursive subdataset found." );
	  }
	}
	bool selecting( true );
	if( m_used_datasets.find( dset )!=m_used_datasets.end() ) {
	  selecting = false;
	}
	m_used_datasets[dset].first = new Path( display_path ); m_used_datasets[dset].second[logical_path] = cdepth;
	m_path_stack.push_back( path );
	log( 1, "Scanning dataset " + path.asString() );
	for ( Dataset::const_iterator i( dset->begin( m_noinherit ) );
	      i != dset->end( m_noinherit ); ++i ) {
	  log( 1, "Scanning..." );
	  log( 1, "Scanning entry " + (*i).second.get() );
	  Utils::magic_ptr<Attribute> entry_attr( (*i).first->attr2( c_attr_entry ) );
	  if ( entry_attr ) {
	    log( 1, "Not a delete marker." );
	    bool readable;
	    if( m_noinherit ) {
	      readable = dset->myrights( (*i).second, c_attr_entry, m_worker.login() ).have_right( m_read_right );
	    } else {
	      readable = dset->myrights( (*i).second, c_attr_entry, m_worker.login(), (*i).first, entry_attr ).have_right( m_read_right );
	    }
	    if ( !readable ) {
	      log( 1, "No rights." );
	      if ( ( *i ).second.get().empty() ) {
		if ( path == Path( m_base_search, m_worker.login() ) ) {
		  throw Server::Exceptions::no( "Search failed." );
		}
	      }
	      log( 1, "No permission." );
	      continue;
	    }
	    log( 1, "Right check passed." );
	  } else {
	    log( 1, "Delete marker?" );
	    continue;
	  }
	  bool added( false );
	  log( 1, "Checking addition." );
	  if( selecting ) {
	    log( 1, "Checking criteria." );
	    if( !crit || crit->acceptable( m_worker.login(), *(dset->subcontext_pure( m_noinherit )), i, 0 ) ) {
	      //log( 1, "Criteria match." );
	      Utils::magic_ptr<Entry> e( (*i).first );
	      if( m_make_context && !m_context_notify ) {
		log( 1, "Needs myrights population." );
		e = (*i).first->clone( false );
		for( Entry::const_iterator ei( e->begin() ); ei!=e->end(); ++ei ) {
		  (*ei).second->myrights( Utils::magic_ptr<RightList>( new RightList( dset->myrights( (*i).second.get(), (*ei).first, m_worker.login() ) ) ) );
		}
	      }
	      added = true;
	      if( m_depth!=1 ) {
		m_subcontext->add_notrans( path.asString() + ( *i ).second.get(), e );
	      } else {
		m_subcontext->add_notrans( ( *i ).second.get(), e );
	      }
	      if ( m_hard_limit_spec ) {
		if ( m_hard_limit < m_subcontext->size() ) {
		  magic_ptr < Token::PList > l( new Token::PList );
		  l->add( new Token::Atom( "WAYTOOMANY" ) );
		  throw Server::Exceptions::no( l, "Hard limit exceeded." );
		}
	      }
	    }
	  }
	  log( 1, "Checking recursion." );
	  if( ( m_depth == 0 ) || ( cdepth != m_depth ) ) {
	    log( 1, "Need recursion." );
	    Utils::magic_ptr<Attribute> sdset_attr( (*i).first->attr2( c_attr_subdataset ) );
	    if ( sdset_attr ) {
	      log( 1, "Have subdataset attr" );
	      // If we have a value of "." present, we should remember this one.
	      //std::cout << __FILE__ << ":" << __LINE__ << std::endl;
	      //std::cout << "Got subdataset attribute.\n";
	      bool readable;
	      if( m_noinherit ) {
		readable = dset->myrights( (*i).second, c_attr_subdataset, m_worker.login() ).have_right( m_read_right );
	      } else {
		readable = dset->myrights( (*i).second, c_attr_subdataset, m_worker.login(), (*i).first, sdset_attr ).have_right( m_read_right );
	      }
	      if( readable ) {
		log( 1, "Readable." );
		Token::List const & sdset( sdset_attr->value()->toList() );
		Utils::magic_ptr<Token::List> l( new Token::List );
		for( int z(0); z!=sdset.length(); ++z ) {
		  log( 1, "Considering " + sdset.get( z ).asString() );
		  std::string const & s( sdset.get(z).toString().value() );
		  if( s=="." ) {
		    log( 1, "Recursing from " + path.asString() + "/" + (*i).second.get() + " to " + s );
		    if( !added ) m_sdset_subcontext->add_notrans( Path( logical_path + (*i).second ).asString(), (*i).first );
		    build_subcontext_dataset( crit, path + (*i).second, logical_path + (*i).second, display_path + (*i).second, cdepth+1 );
		    l.zap();
		    break;
		  } else if( s.length()==1 ) {
		    if( s[0]=='/' ) {
		      log( 1, "Recursing from " + path.asString() + "/" + (*i).second.get() + " to " + s );
		      if( !added ) m_sdset_subcontext->add_notrans( Path( logical_path.asString() + (*i).second.get() ).asString(), (*i).first );
		      build_subcontext_dataset( crit, Path( "/" ), Path( logical_path.asString() + (*i).second.get() ), Path( "/" ), cdepth+1 );
		      l.zap();
		      break;
		    } else {
		      Path t( path.asString() + s );
		      if( ds.exists( t ) ) {
			if( m_follow_local ) {
			  log( 1, "Recursing from " + path.asString() + "/" + (*i).second.get() + " to " + s );
			  if( !added ) m_sdset_subcontext->add_notrans( Path( logical_path.asString() + (*i).second.get() ).asString(), (*i).first );
			  build_subcontext_dataset( crit, path.asString()+s, Path( logical_path.asString() + (*i).second.get() ), Path( display_path.asString() + s ), cdepth+1 );
			  l.zap();
			  break;
			} else {
			  if( l ) l->add( path.asString()+s );
			}
		      } else {
			Utils::magic_ptr<Token::List> l( new Token::List );
			l->add( "*" );
			l->add( "NO" );
			Utils::magic_ptr<Token::PList> pl( new Token::PList );
			pl->add( "NOEXIST" );
			pl->add( t.asString() );
			l->add( Utils::magic_cast<Token::Token>( pl ) );
			l->add( new Token::QuotedString(  "Entry '" + (*i).second.get() + "' in dataset '" + path.asString() + "' contains invalid subdataset '" + s + "'" ) );
			m_worker.send( l );
		      }
		    }
		  } else if( s.find( "//" ) == 0 || s.find( "acap://" ) == 0 ) {
		    if( l ) l->add( s );
		  } else if( s[0]=='/' ) {
		    Path t( s );
		    if( ds.exists( t ) ) {
		      if( m_follow_local ) {
			log( 1, "Recursing from " + path.asString() + "/" + (*i).second.get() + " to " + s );
			if( !added ) m_sdset_subcontext->add_notrans( Path( logical_path.asString() + (*i).second.get() ).asString(), (*i).first );
			build_subcontext_dataset( crit, t, Path( logical_path.asString() + (*i).second.get() ), t, cdepth+1 );
			l.zap();
			break;
		      } else {
			if( l ) l->add( s );
		      }
		    } else {
		      Utils::magic_ptr<Token::List> l( new Token::List );
		      l->add( "*" );
		      l->add( "NO" );
		      Utils::magic_ptr<Token::PList> pl( new Token::PList );
		      pl->add( "NOEXIST" );
		      pl->add( t.asString() );
		      l->add( magic_cast<Token::Token>( pl ) );
		      l->add( new Token::QuotedString("Entry '" + (*i).second.get() + "' in dataset '" + path.asString() + "' contains invalid subdataset '" + s + "'") );
		      m_worker.send( l );
		    }
		  } else if( s.find("../")==0 ) {
		    std::list<Infotrope::Utils::StringRep::entry_type> lc( path.asList() );
		    std::list<Infotrope::Utils::StringRep::entry_type> lr( Path( "/"+s ).asList() );
		    while( !lr.empty() ) {
		      if( ".."== lr.front().get() ) {
			lc.pop_back();
		      } else if( "."!=lr.front().get() ) {
			lc.push_back( lr.front() );
		      }
		      lr.pop_front();
		    }
		    Path t( lc );
		    if( ds.exists( t ) ) {
		      if( m_follow_local ) {
			log( 1, "Recursing from " + path.asString() + "/" + (*i).second.get() + " to " + s );
			if( !added ) m_sdset_subcontext->add_notrans( Path( logical_path.asString() + (*i).second.get() ).asString(), (*i).first );
			build_subcontext_dataset( crit, t, Path( logical_path.asString() + (*i).second.get() ), t, cdepth+1 );
			l.zap();
			break;
		      } else {
			if( l ) l->add( t.asString() );
		      }
		    } else {
		      Utils::magic_ptr<Token::List> l( new Token::List );
		      l->add( "*" );
		      l->add( "NO" );
		      Utils::magic_ptr<Token::PList> pl( new Token::PList );
		      pl->add( "NOEXIST" );
		      pl->add( t.asString() );
		      l->add( Utils::magic_cast<Token::Token>( pl ) );
		      l->add( new Token::QuotedString("Entry '" + (*i).second.get() + "' in dataset '" + path.asString() + "' contains invalid subdataset '" + s + "'") );
		      m_worker.send( l );
		    }
		  }
		}
		if( l && l->length() ) {
		  Utils::magic_ptr<Token::List> ll( new Token::List );
		  ll->add( m_toks->ptr(0) );
		  ll->add( "REFER" );
		  ll->add( path.asString()+(*i).second.get() );
		  ll->add( magic_cast<Token::Token>( l ) );
		  m_worker.send( ll );
		}
	      }
	    }
	  }
	  log( 1, "Entry Scan completed" );
	}
	log( 1, "All scan complete." );
      }
      ++cdepth;
      if ( m_depth != 0 ) {
	if ( m_depth < cdepth ) {
	  m_path_stack.pop_back();
	  return ;
	}
      }
      m_path_stack.pop_back();
    }
    
    void dump_subcontext() {
      unsigned long int returning(0);
      if ( m_limit_spec ) {
	if ( m_limit_limit < m_subcontext->size() ) {
	  returning = m_limit_maxret;
	  m_limit_exceeded = true;
	} else {
	  m_limit_spec = false; // Turn it off, no point now.
	}
      } else if ( !m_return ) {
	return ;
      }
      for ( Subcontext::const_iterator i( m_subcontext->begin() );
	    i != m_subcontext->end(); ++i ) {
	if ( m_limit_spec ) {
	  if ( returning == 0 ) {
	    return;
	  }
	  --returning;
	}
	if ( m_return ) {
	  magic_ptr < Token::List > l( new Token::List );
	  l->add( m_toks->ptr( 0 ) );
	  l->add( new Token::Atom( "ENTRY" ) );
	  l->add( new Token::String( ( *i ).second.get() ) );
	  Path source_dset( (*i).first->attr( c_attr_control )->valuestr() );
	  l->add( m_return->fetch_entry_metadata( *( Datastore::datastore_read().dataset( source_dset ) ), ( *i ).first ) );
	  m_worker.send( l, false );
	}
      }
    }
    
    magic_ptr < Criteria::Criterion > parse_criteria( int & i ) {
      if ( i >= m_toks->length() ) {
	return magic_ptr < Criteria::Criterion > ( new Criteria::All );
      }
      
      if ( m_toks->get( i ).toAtom().value() == "ALL" ) {
	return magic_ptr < Criteria::Criterion > ( new Criteria::All );
      } else if ( m_toks->get( i ).toAtom().value() == "NOT" ) {
	return magic_ptr < Criteria::Criterion > ( new Criteria::Not( parse_criteria( ++i ) ) );
      } else if ( m_toks->get( i ).toAtom().value() == "AND" ) {
	magic_ptr < Criteria::Criterion > c1( parse_criteria( ++i ) );
	magic_ptr < Criteria::Criterion > c2( parse_criteria( ++i ) );
	return magic_ptr < Criteria::Criterion > ( new Criteria::And( c1, c2 ) );
      } else if ( m_toks->get( i ).toAtom().value() == "OR" ) {
	magic_ptr < Criteria::Criterion > c1( parse_criteria( ++i ) );
	magic_ptr < Criteria::Criterion > c2( parse_criteria( ++i ) );
	return magic_ptr < Criteria::Criterion > ( new Criteria::Or( c1, c2 ) );
      } else if ( m_toks->get( i ).toAtom().value() == "EQUAL" ) {
	std::string attribute( m_toks->get( ++i ).toString().value() );
	std::string comparator( m_toks->get( ++i ).toString().value() );
	magic_ptr < Token::Token > match( m_toks->ptr( ++i ) );
	if( !match->isNil() ) {
	  match->toString();
	}
	return magic_ptr < Criteria::Criterion > ( new Criteria::Equal( attribute, comparator, match, m_noinherit ) );
      } else if ( m_toks->get( i ).toAtom().value() == "COMPARE" ) {
	std::string attribute( m_toks->get( ++i ).toString().value() );
	std::string comparator( m_toks->get( ++i ).toString().value() );
	magic_ptr < Token::Token > match( m_toks->ptr( ++i ) );
	if( !match->isNil() ) {
	  match->toString();
	}
	return magic_ptr < Criteria::Criterion > ( new Criteria::Compare( attribute, comparator, match, m_noinherit ) );
      } else if ( m_toks->get( i ).toAtom().value() == "PREFIX" ) {
	std::string attribute( m_toks->get( ++i ).toString().value() );
	std::string comparator( m_toks->get( ++i ).toString().value() );
	magic_ptr < Token::Token > match( m_toks->ptr( ++i ) );
	if( !match->isNil() ) {
	  match->toString();
	}
	return magic_ptr < Criteria::Criterion > ( new Criteria::Prefix( attribute, comparator, match, m_noinherit ) );
      } else if ( m_toks->get( i ).toAtom().value() == "SUBSTRING" ) {
	std::string attribute( m_toks->get( ++i ).toString().value() );
	std::string comparator( m_toks->get( ++i ).toString().value() );
	magic_ptr < Token::Token > match( m_toks->ptr( ++i ) );
	if( !match->isNil() ) {
	  match->toString();
	}
	return magic_ptr < Criteria::Criterion > ( new Criteria::Substring( attribute, comparator, match, m_noinherit ) );
      } else if ( m_toks->get( i ).toAtom().value() == "COMPARESTRICT" ) {
	std::string attribute( m_toks->get( ++i ).toString().value() );
	std::string comparator( m_toks->get( ++i ).toString().value() );
	magic_ptr < Token::Token > match( m_toks->ptr( ++i ) );
	if( !match->isNil() ) {
	  match->toString();
	}
	return magic_ptr < Criteria::Criterion > ( new Criteria::CompareStrict( attribute, comparator, match, m_noinherit ) );
      } else if ( m_toks->get( i ).toAtom().value() == "RANGE" ) {
	long unsigned int from( m_toks->get( ++i ).toInteger().value() );
	long unsigned int to( m_toks->get( ++i ).toInteger().value() );
	Utils::magic_ptr<Token::Token> modtime_token( m_toks->ptr( ++i ) );
	if( m_base_search.empty() || m_base_search[0]!='/' ) {
	  if( !modtime_token->isNil() ) {
	    Modtime m( modtime_token->toString().value() );
	    if( m < m_worker.context( m_base_search )->modtime() ) {
	      throw std::runtime_error( "Context modifed since " + m.asString() + " at " + m_worker.context( m_base_search )->modtime().asString() + ", RANGE invalid." );
	    }
	  }
	} else {
	  throw std::runtime_error( "RANGE specified, but searching dataset." );
	}
	/*if( m_make_context && m_context_notify ) {
	  throw std::string( "RANGE not supported for creation of notify contexts." );
	  }*/
	return magic_ptr < Criteria::Criterion > ( new Criteria::Range( from, to ) );
      } else {
	throw Server::Exceptions::bad( istring_convert( "Don't know what " + m_toks->get( i ).toAtom().value() + " means in search key." ) );
      }
    }
    
    void filter_subcontext( magic_ptr < Criteria::Criterion > const & crit ) {
      if ( crit // No criteria.
	   || ( ( m_base_search.empty() || m_base_search[ 0 ] == '/' )   // Context
		&& m_sort )   // Context and a defined sort.
	   ) { // Optimise away case where no filtering required.
	log( 1, "Search is having to sort/filter..." );
	magic_ptr < Subcontext > tmp( m_subcontext );
	if ( m_sort ) {
	  m_subcontext = magic_ptr < Subcontext > ( new Subcontext( m_sort->toList() ) );
	} else {
	  m_subcontext = magic_ptr < Subcontext > ( new Subcontext( tmp->sort() ) );
	}
	if ( crit ) {
	  Criteria::t_iter_range r( crit->acceptable( m_worker.login(), tmp.get() ) );
	  if( !( ( m_base_search.empty() || m_base_search[ 0 ] == '/' )   // Context
		 && m_sort ) ) { // ... And a defined sort.
	    Criteria::t_iter_range::const_iterator rr( r.begin() );
	    ++rr;
	    if( rr==r.end() ) {
	      // Size of 1.
	      // I should put that test into a template function.
	      if( ( (*(r.begin())).first==tmp->begin() )
		  && ( (*(r.begin())).second==tmp->end() ) ) {
		// Probably an ALL. Or something which evaluated to same.
		// No need to copy it.
		m_subcontext = tmp;
		log( 1, "Using shortcut." );
		return;
	      }
	    }
	  }
	  for ( Criteria::t_iter_range::const_iterator rr( r.begin() );
		rr != r.end(); ++rr ) {
	    for ( Subcontext::const_iterator i( ( *rr ).first );
		  i != ( *rr ).second; ++i ) {
	      m_subcontext->add_notrans( ( *i ).second, ( *i ).first );
	    }
	  }
	} else {
	  for ( Subcontext::const_iterator i( tmp->begin() );
		i != tmp->end(); ++i ) {
	    m_subcontext->add_notrans( ( *i ).second, ( *i ).first );
	  }
	}
      } // We have no criteria, and we are not resorting a context. So we can keep our existing subcontext.
      log( 1, "Done filtering." );
    }
    
    void main() {
      Server::Master::master()->log( 1, "SEARCH is starting: " + m_toks->asString() );
      Infotrope::Threading::ReadLock l__inst( Datastore::datastore_read().lock() );
      struct timeval tv_pre;
      gettimeofday( &tv_pre, 0 );
      log( 1, "Parsing arguments." );
      m_base_search = m_toks->get( 2 ).toString().value();
      int i( 3 );
      for ( ; i < m_toks->length(); ++i ) {
	// Basic rule is if we take an argument, we bump i.
	if ( m_toks->get( i ).toAtom().value() == "DEPTH" ) {
	  m_depth = m_toks->get( ++i ).toInteger().value();
	} else if( m_toks->get( i ).toAtom().value() == "XFOLLOW" ) {
	  ++i;
	  if( m_toks->get( i ).toAtom().value() == "IMMEDIATE" ) {
	    // Do nothing, this is the default.
	  } else if( m_toks->get( i ).toAtom().value() == "LOCAL" ) {
	    m_follow_local = true;
	  } else if( m_toks->get( i ).toAtom().value() == "REMOTE" ) {
	    throw std::runtime_error( "Sorry, can't follow remote subdataset links." );
	  } else {
	    throw std::runtime_error( "Sorry, don't understand XFOLLOW specification " + m_toks->get(i).asString() );
	  }
	} else if ( m_toks->get( i ).toAtom().value() == "HARDLIMIT" ) {
	  m_hard_limit_spec = true;
	  m_hard_limit = m_toks->get( ++i ).toInteger().value();
	} else if ( m_toks->get( i ).toAtom().value() == "LIMIT" ) {
	  m_limit_spec = true;
	  m_limit_limit = m_toks->get( ++i ).toInteger().value();
	  m_limit_maxret = m_toks->get( ++i ).toInteger().value();
	} else if ( m_toks->get( i ).toAtom().value() == "MAKECONTEXT" ) {
	  m_make_context = true;
	  while ( !m_toks->get( ++i ).isString() ) {
	    if ( m_toks->get( i ).toAtom().value() == "ENUMERATE" ) {
	      m_context_enumerate = true;
	    } else if ( m_toks->get( i ).toAtom().value() == "NOTIFY" ) {
	      m_context_notify = true;
#ifdef INFOTROPE_ENABLE_XPERSIST
	    } else if( m_toks->get( i ).toAtom().value() == "PERSIST" ) {
	      m_context_persist = true;
	      m_worker.session(); // Force session creation.
#endif
	    } else {
	      throw std::runtime_error( "Unknown context modifier " + m_toks->get( i ).asString() );
	    }
	  }
	  m_context = m_toks->get( i ).toString().value();
	} else if ( m_toks->get( i ).toAtom().value() == "NOINHERIT" ) {
	  m_noinherit = true;
	} else if ( m_toks->get( i ).toAtom().value() == "RETURN" ) {
	  ++i;
	  m_return = new Server::Return( m_toks->get( i ), m_worker.login() );
	} else if ( m_toks->get( i ).toAtom().value() == "SORT" ) {
	  ++i;
	  m_sort = m_toks->ptr( i );
	} else {
	  break;
	}
      }
      if( m_context_enumerate && !m_sort ) {
	throw Server::Exceptions::bad( "Cannot ENUMERATE unSORTed context." );
      }
      log( 1, "Parsing criteria." );
      // The rest is the search criteria.
      magic_ptr < Criteria::Criterion > crit( parse_criteria( i ) );
      if( ++i < m_toks->length() ) {
	throw Server::Exceptions::bad( "Junk following criteria: " + m_toks->get(i).asString() );
      }
      struct timeval tv_pp;
      gettimeofday( &tv_pp, 0 );
      log( 1, "Building subcontext." );
      build_subcontext( crit );
      {
	std::ostringstream ss;
	ss << "Final size of path stack should be 0, is " << m_path_stack.size();
	log( 1, ss.str() );
      }
      log( 1, "Dumping subcontext." );
      dump_subcontext();
      log( 1, "Considering making a context." );
      struct timeval tv_pb;
      gettimeofday( &tv_pb, 0 );
      if ( m_make_context ) {
	log( 1, "Need to create context. Checking existing." );
	if( m_worker.context_exists( m_context ) ) {
	  log( 1, "Context exists, good." );
	  magic_ptr < Server::Context > c( m_worker.context( m_context ) );
	  log( 1, "Shutting down." );
	  c->shutdown();
	  log( 1, "And finishing." );
	  m_worker.context_finished( c );
	}
	log( 1, "Okay, now creating new context." );
	Utils::magic_ptr<Server::Context> c( new Server::Context( m_context, m_subcontext, m_sdset_subcontext, crit, m_return , m_used_datasets, m_base_search, m_depth, m_context_notify, m_context_enumerate, m_context_persist, m_noinherit, m_follow_local, m_worker ) );
	log( 1, "And registering." );
	m_worker.context_register( c );
	log( 1, "Setting up." );
	c->setup();
	log( 1, "Done." );
      }
      log( 1, "Issuing MODTIME." );
      {
	magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "MODTIME" ) );
	l->add( new Token::String( Modtime::modtime().asString() ) );
	m_worker.send( l, false );
      }
      log( 1, "And OK." );
      {
	struct timeval tv_final;
	gettimeofday( &tv_final, 0 );
	std::ostringstream ss;
	struct timeval tv_build_time;
	ss << "From " << tv_pp.tv_sec << "." << tv_pp.tv_usec << " to " << tv_pb.tv_sec << "." << tv_pb.tv_usec << " - ";
	tv_build_time.tv_usec = 0;
	tv_build_time.tv_usec = tv_pb.tv_usec;
	tv_build_time.tv_usec += 1000000 * ( tv_pb.tv_sec - tv_pp.tv_sec );
	tv_build_time.tv_usec -= tv_pp.tv_usec;
	ss << "Search took " << (tv_build_time.tv_usec / 1000 ) << "ms.";
	magic_ptr < Token::List > l( new Token::List );
	l->add( m_toks->ptr( 0 ) );
	l->add( new Token::Atom( "OK" ) );
	if ( m_limit_exceeded ) {
	  Token::PList * pl( new Token::PList );
	  pl->add( new Token::Atom( "TOOMANY" ) );
	  pl->add( new Token::Integer( m_subcontext->size() ) );
	  l->add( pl );
	}
	l->add( new Token::String( ss.str() ) );
	m_worker.send( l, true );
      }
      log( 1, "SEARCH complete." );
    }
  };
  
  Server::Command::Register<Search> f( "SEARCH", Server::AUTH );
}
