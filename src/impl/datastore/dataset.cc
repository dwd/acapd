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
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/entry.hh>
#include <infotrope/datastore/exceptions.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/datastore/comparator.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/server/master.hh>
#include <infotrope/utils/utf8.hh>

using namespace Infotrope::Data;
using namespace Infotrope::Server;
using namespace Infotrope::Utils;
using namespace Infotrope::Constants;

// #define DSET_DEBUG

#ifdef DSET_DEBUG
#define DSET_LOG( x, y ) Master::master()->log( (x), std::string(y) + " " + __FUNCTION__ )
#else
#define DSET_LOG( x, y ) (void)(x)
#endif

Dataset::Dataset( Datastore & ds, Path const & path,
                  magic_ptr<Subcontext> const & sc,
                  magic_ptr<Subcontext> const & req,
                  magic_ptr<Subcontext> const & ov )
  : Notify::Source(),
    m_requested_subcontext( req ),
    m_subcontext( sc ),
    m_overlay_subcontext( ov ),
    m_path( path.asString() ),
    m_trans_path( path.asString() ),
    m_datastore( ds ),
    m_inherit_changed( false ),
    m_deletion( false ),
    m_last_deletion( Modtime::modtime() ),
    m_erasing( false ) {
}

void Dataset::setup() {
  DSET_LOG( 3, "Setup for " + m_trans_path.asString() );
  TRANSACTION_VALIDATE();
  if ( !m_requested_subcontext->exists2( c_entry_empty ) ) {
    magic_ptr<Entry> e( new Entry( c_entry_empty ) );
    e->add( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) );
    DSET_LOG( 1, "Selecting default ACL." );
    Utils::magic_ptr<Acl> acl;
    if( m_trans_path == "/" ) {
      DSET_LOG( 1, "Top path, generating basic ACL." );
      // Special case this.
      Utils::magic_ptr<Token::Token> tmp( new Token::PList );
      tmp->toList().add( new Token::String( "anyone\trx" ) );
      acl = new Acl( tmp );
    } else {
      // Find suitable parent.
      Path p( m_trans_path );
      if( p.isView( Path::ByClass ) ) {
        if( p.translatable( Path::ByOwner ) ) {
          p=p.translate( Path::ByOwner ).parent();
        } else {
          p = p.parent();
        }
      } else {
        p = p.parent();
      }
      if( m_trans_path.translatable( Path::ByClass ) ) {
        // Ah, goodie.
        Utils::magic_ptr<Entry> tmp( m_datastore.dataset( p )->fetch2( c_entry_empty ) );
        if( !tmp || !tmp->exists( c_attr_dataset_acl ) ) {
          m_datastore.dataset( p )->poke( c_entry_empty );
          tmp = m_datastore.dataset( p )->fetch2( c_entry_empty );
        }
        if( !tmp ) {
          throw std::string( "Byowner parent " + p.asString() + " has no empty entry!" );
        }
        if( !tmp->exists( c_attr_dataset_acl ) ) {
          throw std::string( "Byowner parent " + p.asString() + " has no dataset.acl." );
        }
        // TODO - copy all dataset.acl.* attributes.
        DSET_LOG( 1, "Using byowner parent." );
        acl = new Acl( tmp->attr( c_attr_dataset_acl )->value_acl()->tokens() );
      } else {
        DSET_LOG( 1, "Generating on position." );
        // It's not translatable. If it's /byowner/<owner thingy>...
        bool anyone( false );
        bool noanon( false );
        if( p.size()==0 ) {
          // /byowner/ or /dataset-class/
          anyone=true;
        } else if( p.size()==1 ) {
          if( m_trans_path.mybit()==std::string("site") ) {
            anyone=true;
            noanon=true;
          }
          // /{byowner|dataset class}/{group|user}/ shouldn't be readable by 'anyone'...
        }
        Utils::magic_ptr<Token::Token> tmp( new Token::PList() );
        tmp->toList().add( new Token::String( "$owner\trwixa" ) );
        if( anyone ) {
          tmp->toList().add( new Token::String( "anyone\trx" ) );
        }
        if( noanon ) {
          tmp->toList().add( new Token::String( "-anonymous\trx" ) );
        }
        acl = new Acl( tmp );
        if( !anyone ) { // What was this for again? - Oh yes. /dataset-class/user/ needs to be visible.
          if( e->attr( c_attr_entry )->acl()->isNil() ) {
            if( p.size()==1 ) {
              Utils::magic_ptr<Acl> acl2( new Acl() );
              acl2->grant( c_authid_owner, RightList( "rwixa" ) );
              acl2->grant( c_authid_anyone, RightList( "rx" ) );
              e->attr( c_attr_entry )->acl( acl2 );
              e->attr( c_attr_entry )->active_acl( acl2 );
            }
          }
        }
      }
    }
    acl->name( m_trans_path.asString() );
    e->add( Utils::magic_ptr<Attribute>( new Attribute( c_attr_dataset_acl, acl ) ) );
    Transaction::add( m_requested_subcontext );
    m_requested_subcontext->add( c_entry_empty, e );
    DSET_LOG( 1, "Got default ACL: " + acl->tokens()->asString() );
  }
  if ( m_trans_path.isView( Path::ByOwner ) && m_trans_path != "/" ) {
    DSET_LOG( 1, "Generating inheritance." );
    if ( !m_requested_subcontext->fetch2( c_entry_empty )->exists( c_attr_dataset_inherit ) ) {
      Utils::magic_ptr<Dataset> dset( m_datastore.dataset( m_trans_path.parent() ) );
      Utils::magic_ptr<Entry> parent_empty( dset->fetch2( c_entry_empty, true ) );
      if ( parent_empty->exists( c_attr_dataset_inherit ) ) {
        Path pinh( parent_empty->attr( c_attr_dataset_inherit )->valuestr() );
        Path inh( pinh.translate( Path::ByOwner ).asString() + m_trans_path.mybit().get() + "/" );
        magic_ptr<Entry> e( new Entry( c_entry_empty ) );
        e->add( new Attribute( c_attr_dataset_inherit, inh.asString() ) );
        e->merge( m_requested_subcontext->fetch2( c_entry_empty ), false );
        e->add( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) );
        m_requested_subcontext->add( c_entry_empty, e );
        inherit_set( inh );
      }
    }
  }
  Utils::magic_ptr<Entry> empty( m_requested_subcontext->fetch2( c_entry_empty ) );
  if( !empty->exists( c_attr_dataset_inherit_modtime ) ) {
    empty->add( new Attribute( c_attr_dataset_inherit_modtime, Transaction::modtime().asString() ) );
  }
  // Look for datasets which inherit from this one.
  DSET_LOG( 1, "Scanning for children of " + m_trans_path.asString() );
  for ( Datastore::const_iterator i( m_datastore.begin() );
        i != m_datastore.end();
        ++i ) {
    DSET_LOG( 1, "Scanning child " + (*i).first.get() );
    Utils::magic_ptr<Entry> test( (*i).second->fetch2( c_entry_empty, true ) );
    if( test ) {
      if( test->exists( c_attr_dataset_inherit ) ) {
        if( test->attr( c_attr_dataset_inherit )->valuestr() == m_trans_path.asString() ) {
          // Found one. Reset inheritance. Really, this should only poke "", but
          // there's no existing shortcut for that.
          // TODO OPT
          DSET_LOG( 1, "Found child " + (*i).first.get() );
          ( *i ).second->inherit_reset();
          ( *i ).second->inherit_recalculate();
          DSET_LOG( 1, "CHild of " + m_trans_path.asString() + " which is " + (*i).first.get() + " set, children now " + fetch2( c_entry_empty, true )->attr( c_attr_dataset_children )->value()->asString() );
        }
      }
    }
  }
  DSET_LOG( 1, "First poke." );
  poke( c_entry_empty );
  DSET_LOG( 1, "Inheritance recalc..." );
  inherit_recalculate();
  DSET_LOG( 3, "Setup complete." );
}

bool Dataset::isNormal() {
  return !exists2( c_entry_empty, true ) || !fetch2( c_entry_empty, true )->exists( c_attr_dataset_type );
}

void Dataset::store( magic_ptr<Token::List> const & a, Utils::StringRep::entry_type const & u ) {
  TRANSACTION_VALIDATE();
  if( !myrights( c_entry_empty, c_attr_entry, u ).have_right( 'r' ) ) {
    throw std::runtime_error( "Failure while trying to create dataset." );
  }
  // First things first. If we're a shadow or a virtual, promote ourselves by Devious Means.
  if ( !isNormal() ) {
    m_datastore.create( m_trans_path );
    // This should promote us as needs be.
  }
  
  // Okay, we're real. (And keeping it as such, give it up to the posse, etc)
  
  {
    std::string const & name( a->get( 0 ).toString().value() );
    if( name.length() ) {
      if( name[0]=='.' ) {
        throw Exceptions::invalid_attr( "Entry cannot begin with '.'" );
      }
      if( name.find( '/' )!=std::string::npos ) {
        throw Exceptions::invalid_attr( "Entry cannot contain '/'" );
      }
    }
  }
  bool entry_acl_modified( false );
  Utils::StringRep::entry_type entry( a->get( 0 ).toString().value() );
  // If there is no entry already, we'll need to create one.
  magic_ptr<Entry> e( new Entry() );
  
  magic_ptr<Entry> old( m_subcontext->fetch2( entry, true ) );
  magic_ptr<Attribute> old_entry;
  if( old ) {
    old_entry = old->attr2( c_attr_entry );
  }
  magic_ptr<Entry> old_nih( m_overlay_subcontext->fetch2( entry, true ) );
  if( old ) {
    // Entry exists in some way.
    if( myrights( entry, c_attr_entry, u, old, old_entry ).have_right( 'r' ) ) {
      // And we allowed to acknowledge that.
    } else {
      // Ah... Does it exist at all?
      if( old_nih ) {
        // Yes - so we need to explode. Note that giving the real reason would mean acknowledging its existence, so we give a drab error.
        std::runtime_error( "Store failed, cannot commit." );
      } else if( myrights( entry, c_attr_entry, u, old, old_entry ).have_right( 'i' ) ) {
        // Pretend we're adding a new one - we'll simply create an overlay entry, and the user will never be any the wiser.
      } else {
        std::runtime_error( "Entry creation failed, cannot commit." );
      }
    }
  }
  
  bool inherit_changing( false );
  for ( int i( 1 ); i < a->length(); i += 2 ) {
    magic_ptr<Attribute> attr;
    magic_ptr<Acl> newacl;
    Utils::StringRep::entry_type attr_name( a->get( i ).toString().value() );
    magic_ptr<Attribute> old_attr;
    if( old ) {
      old_attr = old->attr2( attr_name );
    }
    bool modify( false );
    bool modify_acl( false );
    
    if( e->exists_pure( attr_name ) ) {
      throw Exceptions::proto_format_error( "Attribute listed twice in store-list" );
    }
    
    if( old_nih && old_nih->exists( attr_name ) && myrights( entry, attr_name, u, old, old_attr ).have_right( 'r' ) ) {
      e->add( old_nih->attr( attr_name )->clone() );
    } else {
      e->add( new Attribute( attr_name, magic_ptr<Token::Token>() ) );
    }
    if( e->attr( attr_name )->acl()->isNil() ) {
      modify_acl = true;
    }
    if( e->attr( attr_name )->value()->isNil() ) {
      modify_acl = true;
    }
    bool aclcheck( false );
    if ( entry->empty() ) {
      if ( attr_name == c_attr_dataset_acl || attr_name->find( "dataset.acl." ) == 0 ) {
        if ( !myrights( entry, attr_name, u, old, old_attr ).have_right( 'a' ) ) {
          throw Data::Exceptions::no_permission( *acl( entry, attr_name, old, old_attr ), m_trans_path.asString() + entry.get() + "/" + attr_name.get(), Right::right( 'a' ), " Needed to manipulate dataset.acl*" );
        }
        aclcheck = true;
      } else if ( attr_name == c_attr_dataset_inherit ) {
        inherit_changing = true;
      }
    }
    if ( attr_name->find( "vendor.infotrope.system." )==0 ) {
      throw std::string( "Cannot change system attribute '" + attr_name.get() );
    }
    //if ( attr_name == c_attr_modtime ) {
    //  throw std::string( "Cannot change system attribute modtime." );
    //}
    for ( int x( 0 ); x < a->get( i + 1 ).toList().length(); x += 2 ) {
      if ( a->get( i + 1 ).toList().get( x ).toString().value() == "value" ) {
        DSET_LOG( 2, "Changing attribute "+attr_name.get()+" to "+a->get(i+1).toList().get(x+1).asString() );
        if ( !myrights( entry, attr_name, u, old, old_attr ).have_right( modify ? 'w' : 'i' ) ) {
          throw Data::Exceptions::no_permission( *acl( entry, attr_name, old, old_attr ), m_trans_path.asString() + entry.get() + "/" + attr_name.get(), Right::right( modify ? 'w' : 'i' ), " Needed for STORE on value." );
        }
        if( a->get( i + 1 ).toList().get( x + 1 ).isNil() ) {
          e->store_nil( attr_name );
        } else if ( a->get( i + 1 ).toList().get( x + 1 ).isString() ) {
          if( attr_name == c_attr_dataset_inherit ) {
            try {
              Path p( a->get( i+1 ).toList().get( x+1 ).toString().value(), u );
              e->add( new Attribute( attr_name, p.canonicalize().asString() ) );
            } catch( Data::Exceptions::invalid_path & e ) {
              throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), attr_name.get(), e.what() );
            }
          } else if( aclcheck ) {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), attr_name.get(), "ACL attribute must be multivalued" );
          } else {
            e->add( new Attribute( attr_name,
                                   a->get( i + 1 ).toList().ptr( x + 1 ) ) );
          }
        } else if ( a->get( i + 1 ).toList().get( x + 1 ).isList() ) {
          if( attr_name == c_attr_dataset_inherit ) {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), attr_name.get(), "Multiple inheritance not supported." );
          }
          if( aclcheck ) {
            Utils::magic_ptr<Acl> acl( new Acl( a->get( i + 1 ).toList().ptr( x + 1 ) ) );
            if( attr_name == c_attr_dataset_acl ) {
              acl->name( path().asString() );
            } else {
              acl->name( path().asString(), attr_name.get().substr( sizeof("dataset.acl.") ) );
            }
            e->add( new Attribute( attr_name, acl ) );
          } else {
            e->add( new Attribute( attr_name,
                                   a->get( i + 1 ).toList().ptr( x + 1 ) ) );
          }
        } else if ( a->get( i + 1 ).toList().get( x + 1 ).isAtom() ) {
          aclcheck = false;
          DSET_LOG( 1, "Storing " + a->get( i + 1 ).toList().get( x + 1 ).asString() + " to " + attr_name.get() );
          if ( a->get( i + 1 ).toList().get( x + 1 ).toAtom().value() == "DEFAULT" ) {
            e->store_default( attr_name );
          } else {
            throw std::runtime_error( "Don't know what " + a->get( i + 1 ).toList().get( x + 1 ).asString() + " means as value specifier." );
          }
        }
        if ( aclcheck ) {
          try {
            Acl a1( a->get( i + 1 ).toList().ptr( x + 1 ) );
            // Creates and destroys ACL, which should throw if it's
            // the wrong format.
          } catch( std::string const & e ) {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), attr_name.get(), e );
          }
        }
      } else if ( a->get( i + 1 ).toList().get( x ).toString().value() == "acl" ) {
        if( attr_name == c_attr_entry ) {
          entry_acl_modified = true;
        }
        if ( !myrights( entry, attr_name, u, old, old_attr ).have_right( 'a' ) ) {
          throw Data::Exceptions::no_permission( *acl( entry, attr_name, old, old_attr ), m_trans_path.asString() + entry.get() + "/" + attr_name.get(), Right::right( 'a' ), " Needed to STORE acl metadata." );
        }
        if( a->get( i + 1 ).toList().get( x+1 ).isNil() ) {
          e->store_acl_nil( attr_name );
        } else if( a->get( i + 1 ).toList().get( x+1 ).isAtom() ) {
          if( a->get( i+1 ).toList().get( x+1 ).toAtom().value()=="DEFAULT" ) {
            e->store_acl_default( attr_name );
          } else {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), attr_name.get(), "Not a valid ACL" );
          }
        } else {
          e->attr( attr_name )->acl( magic_ptr < Acl > ( new Acl( a->get( i + 1 ).toList().ptr( x + 1 ) ) ) );
        }
      } else {
        throw Data::Exceptions::proto_format_error( "Cannot set attribute metadata " + a->get( i + 1 ).toList().get( x ).toString().value() );
      }
    }
  }
  if( entry_acl_modified ) {
    if( e->attr_isdefault( c_attr_entry ) ) {
      e->attr( c_attr_entry )->value( Utils::magic_ptr<Token::Token>( new Token::String( entry.get() ) ) );
    }
  }
  // We now have a delta fully built.
  // Actually add the entry:
  add2( e, entry );
}

void Dataset::add2( magic_ptr< Entry > const & e ) {
  add2( e, e->attr( c_attr_entry ) ->valuestr() );
}

magic_ptr<Dataset> Dataset::dataset_inherit() const {
  Utils::magic_ptr<Dataset> ds;
  Utils::magic_ptr<Entry> e( m_requested_subcontext->fetch2( c_entry_empty, true ) );
  if( e ) {
    if( e->exists( c_attr_dataset_inherit ) ) {
      Path p( e->attr( c_attr_dataset_inherit )->valuestr() );
      if( m_datastore.exists( p ) ) {
        ds = m_datastore.dataset( p );
      }
    }
  }
  return ds;
}

Infotrope::Utils::StringRep::entry_type Dataset::acl_get_owner( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const & e, Utils::magic_ptr<Attribute> const & a ) const {
  //Utils::magic_ptr<Entry> e( fetch2( entry ) );
  if( a ) {
    Path const & path( a->origin() );
    if( path.has_owner() ) {
      return path.owner();
    }
    if( entry != c_entry_empty ) {
      Path p( path + entry );
      if( p.has_owner() ) {
        return p.owner();
      }
    }
    return c_authid_owner;
  } else {
    if( path().has_owner() ) {
      return path().owner();
    }
  }
  return c_authid_owner;
}

//magic_ptr<Acl> Dataset::acl( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr ) const {
//  return acl( entry, attr, fetch2( entry ) );
//}

magic_ptr<Acl> Dataset::acl( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const & e, Utils::magic_ptr<Attribute> const & a ) const {
  if( Transaction::mine() ) {
    return acl_find( entry, attr, e, a );
  } else {
    DSET_LOG( 1, "Trying active_acl." );
    if( a ) {
      if( !a->active_acl()->isNil() ) {
        DSET_LOG( 1, "Got it." );
        return a->active_acl();
      } else {
        DSET_LOG( 4, "Oooops: Active ACL is NIL!" );
        return a->active_acl( acl_find( entry, attr, e, a ) );
      }
    }
    if( m_acl_cache[ entry ][ attr ]->isNil() ) {
      m_acl_cache[entry][attr] = acl_find( entry, attr, e, a );
    }
    return m_acl_cache[entry][attr];
  }
}

magic_ptr<Acl> Dataset::acl_find( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::magic_ptr<Entry> const & cand, Utils::magic_ptr<Attribute> const & a ) const {
  if( a ) {
    if( !a->acl()->isNil() ) {
      Utils::magic_ptr<Acl> acl( a->acl() );
      acl->name( path().asString(), attr.get(), entry.get() );
      return acl;
    }
  }
  Utils::magic_ptr<Entry> empty( m_requested_empty );
  if( Transaction::mine() ) {
    empty = m_requested_subcontext->fetch2( c_entry_empty, true );
  }
  if ( !empty ) {
    DSET_LOG( 1, "** Synthesising maximally restrictive ACL for " + path().asString() + "/" + entry.get() + "//" + attr.get() );
    Utils::magic_ptr<Token::Token> pl( new Token::PList );
    pl->toList().add( new Token::String( "-anyone\trwixa" ) );
    Utils::magic_ptr<Acl> acl( new Acl( pl ) );
    acl->name( path().asString() );
    return acl;
  }
  Utils::magic_ptr<Attribute> a2( empty->attr2( "dataset.acl." + attr.get() ) );
  if( !a2 ) {
    a2 = empty->attr2( c_attr_dataset_acl );
  }
  if( a2 ) {
    return a2->value_acl();
  }
  // If we got here, then the dataset.acl attribute for this dataset does not
  // exist. This is a fault, since it "must" (in small letters).
  // There is no indication of how a dataset might actually obtain such an
  // attribute, so we've guessed. We simply make one up.
  throw Data::Exceptions::proto_format_error( "No dataset.acl attribute defined in " + path().asString() );
}

RightList Dataset::myrights( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::StringRep::entry_type const & user ) const {
  Utils::magic_ptr<Entry> const & e( fetch2( entry ) );
  return myrights( entry, attr, user, e, e->attr2( attr ) );
}

RightList Dataset::myrights( Utils::StringRep::entry_type const & entry, Utils::StringRep::entry_type const & attr, Utils::StringRep::entry_type const & user, Utils::magic_ptr<Entry> const & e, Utils::magic_ptr<Attribute> const & a ) const {
  DSET_LOG( 1, "Finding ACL owner." );
  Utils::StringRep::entry_type owner( acl_get_owner( entry, attr, e, a ) );
  DSET_LOG( 1, "Finding MYRIGHTS." );
  return acl( entry, attr, e, a )->myrights( user, owner );
}

void Dataset::inherit_clear() {
  DSET_LOG( 1, "Clearing inheritance for " + m_trans_path.asString() );
  Utils::magic_ptr<Entry> em( m_requested_subcontext->fetch2( c_entry_empty, true ) );
  if ( em && em->exists( c_attr_dataset_inherit ) ) {
    Path inh1( em->attr( c_attr_dataset_inherit )->valuestr() );
    Path inh( inh1.translate( Path::ByOwner ) );
    Utils::magic_ptr<Entry> e( new Entry( c_entry_empty ) );
    e->add( new Attribute( c_attr_dataset_inherit_modtime, Transaction::modtime().asString() ) );
    add2( e, c_entry_empty );
    
    if ( m_datastore.exists( inh ) ) {
      // Parent dataset exists, we should remove the child entry if it exists.
      Utils::magic_ptr<Entry> ec( m_datastore.dataset( inh )->m_requested_subcontext->fetch2( c_entry_empty, true ) );
      if ( ec && ec->exists( c_attr_dataset_children ) ) {
        // ... Which it does. Copy the old one.
        Utils::magic_ptr< Token::List > l( magic_cast<Token::List>( ec->attr( c_attr_dataset_children )->value() ) );
        Utils::magic_ptr< Entry > newe( new Entry( c_entry_empty ) );
        newe->add( new Attribute( c_attr_dataset_children, magic_ptr<Token::Token>( new Token::PList ) ) );
        for ( int i( 0 ); i < l->length(); ++i ) {
          if ( Path( l->get( i ).toString().value() ).translate( Path::ByOwner )
               != m_trans_path.translate( Path::ByOwner ) ) {
            newe->attr( c_attr_dataset_children )->value()->toList().add( l->ptr( i ) );
          }
        }
        m_datastore.dataset( inh )->add2( newe, c_entry_empty );
      }
    }
    m_inherit_changed = true;
  }
}


void Dataset::inherit_reset() {
  inherit_clear();
  Utils::magic_ptr<Entry> em( m_requested_subcontext->fetch2( c_entry_empty, true ) );
  if( em ) {
    if( em->exists( c_attr_dataset_inherit ) ) {
      inherit_set( em->attr( c_attr_dataset_inherit )->valuestr() );
    }
  }
  inherit_recalculate();
}

void Dataset::inherit_set( Path const & p ) {
  DSET_LOG( 5, "Setting inheritance for " + m_trans_path.asString() + " to " + p.asString() );
  p.validate();
  if ( p == m_trans_path ) {
    throw Data::Exceptions::proto_format_error( "Inheritance loop?" );
  }
  Path path( p.translate( Path::ByOwner ) );
  if ( path == m_trans_path.translate( Path::ByOwner ) ) {
    throw Data::Exceptions::proto_format_error( "Inheritance loop?" );
  }
  inherit_clear();
  if ( !m_datastore.exists( p ) ) {
    DSET_LOG( 1, "New parent does not exist." );
    return ;
  }
  m_inherit_changed = true;
  Utils::magic_ptr<Dataset> dset( m_datastore.dataset( p ) );
  Utils::magic_ptr<Entry> e2( dset->fetch2( c_entry_empty, true ) );
  if( !e2 ) {
    return;
  }
  Utils::magic_ptr<Entry> e( new Entry( c_entry_empty ) );
  e->add( new Attribute( c_attr_dataset_inherit_modtime, Transaction::modtime().asString() ) );
  if ( e2->exists( c_attr_dataset_children ) ) {
    // Copy it.
    e->add( e2->attr( c_attr_dataset_children )->clone() );
    e->attr( c_attr_dataset_children )->add( m_trans_path.translate( Path::ByOwner ).asString() );
  } else {
    Token::PList * pl( new Token::PList );
    pl->add( new Token::String( m_trans_path.translate( Path::ByOwner ).asString() ) );
    e->add( new Attribute( c_attr_dataset_children, Utils::magic_ptr < Token::Token > ( pl ) ) );
  }
  DSET_LOG( 1, "Adding child entry, now have " + e->attr( c_attr_dataset_children )->value()->asString() );
  dset->add2( e, c_entry_empty );
  // Slap in a temporary - this will always be canonicalized later.
  // In reality, it's *only* used to detect inheritance loops.
  Utils::magic_ptr<Entry> em( m_requested_subcontext->fetch2( c_entry_empty, true ) );
  if( !em ) {
    return;
  }
  if( em->exists( c_attr_dataset_inherit ) ) {
    em->attr( c_attr_dataset_inherit )->value( Utils::magic_ptr<Token::Token>( new Token::String( p.asString() ) ) );
  } else {
    em->add( new Attribute( c_attr_dataset_inherit, p.asString() ) );
  }
  m_inherit_changed = true;
  Transaction::add( m_trans_path );
  
  // Now update children.
  
  if ( em->exists( c_attr_dataset_children ) ) {
    magic_ptr<Token::List> l( magic_cast<Token::List>( em->attr( c_attr_dataset_children )->value() ) );
    em->store_default( c_attr_dataset_children );
    for ( int i( 0 ); i < l->length(); ++i ) {
      Path p( l->get( i ).toString().value() );
      if ( m_datastore.exists( p ) ) {
        m_datastore.dataset( p )->inherit_reset();
      }
    }
  }
}

void Dataset::commit( Modtime const & m ) {
  if ( m_trans_path != m_path ) {
    m_path = m_trans_path;
  }
  if( m_deletion ) {
    m_deletion = false;
    m_last_deletion = m;
  }
  m_inherit_changed = false;    
  m_acl_cache.clear();
  m_empty = m_subcontext->fetch2( c_entry_empty, true );
  m_overlay_empty = m_overlay_subcontext->fetch2( c_entry_empty, true );
  m_requested_empty = m_requested_subcontext->fetch2( c_entry_empty, true );
}

void Dataset::post_commit( Utils::magic_ptr<Dataset> const & me, Modtime const & m ) {
  if ( !m_poked.empty() ) {
    for ( t_poked::const_iterator i( m_poked.begin() );
            i != m_poked.end(); ++i ) {
#if 0
      // Do metadata stuff here as well. Buggered if I can get it working elsewhere.
      if( exists( *i, true ) ) {
        Utils::magic_ptr<Entry> e( fetch( *i, true ) );
        Utils::magic_ptr<Entry> er;
        if( exists( *i ) ) er = fetch( *i );
        for( Entry::const_iterator ei( e->begin() ); ei!=e->end(); ++ei ) {
          Utils::magic_ptr<Acl> a( acl_find( *i, (*ei).first, e ) );
          e->attr( (*ei).first )->active_acl( a );
          if( er && er->exists( (*ei).first ) ) er->attr( (*ei).first )->active_acl( a );
        }
      }
#endif
      notify( Utils::magic_cast<Notify::Source>( me ), (*i), m, 0 );
    }
    m_poked.clear();
  }
}

void Dataset::rollback() {
  m_inherit_changed = false;
  m_deletion = false;
  m_trans_path = m_path;
  m_poked.clear();
}

static int g_depth( 0 );

namespace {
  class Spacer {
  public:
    Spacer( char c='@' ) {
      ++g_depth;
      if( g_depth > 30 ) {
        int *p(0);
        throw std::string( *p ? "Poke depth exceeded" : 0 );
      }
      for ( int i( 0 ); i < g_depth; ++i ) {
        spacer += c;
      }
      spacer += ' ';
    }
    ~Spacer() {
      --g_depth;
    }
    std::string spacer;
  };
  std::ostream & operator<<( std::ostream & o, Spacer const & c ) {
    return o << c.spacer;
  }
}

void Dataset::poke( Utils::StringRep::entry_type const & entry ) {
  Spacer spacer;
  DSET_LOG( 2, spacer.spacer + "poke: " + m_trans_path.asString() + ", entry '" + entry.get() + "'" );
  if( poke_child( entry ) ) {
    if( poke_data( entry ) ) {
      if( entry == c_entry_empty ) {
        DSET_LOG( 2, spacer.spacer + "poke for empty, needs rerun." );
        poke_child( c_entry_empty );
        poke_data( c_entry_empty );
      }
    }
  } else {
    poke_data( entry );
  }
  DSET_LOG( 2, spacer.spacer + "poke complete: " + m_trans_path.asString() + ", entry '" + entry.get() + "'" );
}

bool Dataset::poke_child( Utils::StringRep::entry_type const & entry ) {
  Spacer spacer;
  bool fail( false );
  
  DSET_LOG( 1, spacer.spacer + "poke_child for " + m_trans_path.asString() + ", entry '" + entry.get() + "'" );
  
  if( entry != c_entry_empty ) {
    DSET_LOG( 1, spacer.spacer + "Need to poke empty entry." );
    poke( c_entry_empty );
  }
  
  Utils::magic_ptr<Entry> nih_e( m_overlay_subcontext->fetch2( entry, true ) );
  Utils::magic_ptr<Entry> rq_e( m_requested_subcontext->fetch2( entry, true ) );
  
  if( nih_e ) {
    //&& fetch( entry, true )->exists( c_attr_entry_rtime ) ) {
    if( Modtime( nih_e->attr( c_attr_entry_rtime )->valuestr() ) == Transaction::poke_modtime() ) {
      DSET_LOG( 1, spacer.spacer + "poke_child: no poke needed." );
      return fail;
    }
    if( rq_e ) {
      if( !rq_e->exists( c_attr_entry_rtime ) ) {
        rq_e->add( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) );
        m_requested_subcontext->add( entry, rq_e );
      }
      if( Modtime( rq_e->attr( c_attr_entry_rtime )->valuestr() ) < Modtime( nih_e->attr( c_attr_entry_rtime )->valuestr() ) ) {
        DSET_LOG( 1, spacer.spacer + "poke_child: no poke needed [MK]" );
        return fail;
      }
    }
  }
  
  m_poked.insert( entry );
  if( m_datastore.exists( m_trans_path ) ) {
    Transaction::add( m_trans_path );
  }
  
  if( !rq_e ) {
    if( nih_e ) {
      subcontext_pure( true )->remove( entry );
      DSET_LOG( 1, spacer.spacer + "poke_child: Deleted overlay entry, poke done." );
      return true;
    } else if( dataset_inherit() ) {
      return true; // Wholly inherited?
    } else {
      DSET_LOG( 1, spacer.spacer + "poke_child: Nothing exists, nothing to do." );
      return fail;
    }
  }
  
  Utils::magic_ptr<Entry> ce( new Entry() );
  ce->merge( rq_e, false );

  // Add any missing but required attributes.
  if( !ce->exists_pure( c_attr_entry ) ) {
    ce->add( new Attribute( c_attr_entry, entry.get() ) );
  }
  //ce->add( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) );
  ce->add( new Attribute( c_attr_modtime, Transaction::modtime().asString() ) );
  ce->add( new Attribute( c_attr_control, m_trans_path.asString() ) );
  // Strip out any DEFAULT attributes.
  {
    bool flag( true );
    do {
      flag = false;
      for( Entry::const_iterator i( ce->begin() ); i!=ce->end(); ++i ) {
        if( ce->attr_isdefault( (*i).first ) ) {
          ce->erase( (*i).first );
          flag = true;
          break;
        }
      }
    } while( flag );
  }
      
  // Now look through and add any missing but required metadata.
  
  //Utils::magic_ptr<Entry> cand;
  //if( entry == c_entry_empty ) {
  //  cand = ce;
  //}
  for( Entry::const_iterator i( ce->begin() ); i!=ce->end(); ++i ) {
    DSET_LOG( 1, spacer.spacer + "poke_child: Metadata processing for " + (*i).first.get() );
    (*i).second->myrights( Utils::magic_ptr<RightList>() );
    (*i).second->origin( m_trans_path );
    (*i).second->size_calc();
    try {
      // We pass in overlay here, which is normally incorrect.
      (*i).second->active_acl( acl_find( entry, (*i).first, ce, (*i).second ) );
    } catch( Data::Exceptions::proto_format_error & e ) {
      DSET_LOG( 1, spacer.spacer + "poke_child: missing dataset.acl problem ignored." );
    }
    DSET_LOG( 1, spacer.spacer + "poke_child: Now have active_acl: " + ce->attr( (*i).first )->active_acl()->tokens()->asString() );
  }
  
  // Finally, add it to our overlay.
  
  DSET_LOG( 2, spacer.spacer + "Adding entry to overlay." );
  subcontext_pure( true )->add( entry, ce );
  Transaction::add( subcontext_pure( true ) );
  
  DSET_LOG( 1, spacer.spacer + "poke_child: Done." );
  
  return true;
}

bool Dataset::poke_data( Utils::StringRep::entry_type const & entry ) {
  Spacer spacer;
  DSET_LOG( 3, spacer.spacer + "Poking entry " + entry.get() + " in " + m_trans_path.asString() );
  if( entry != c_entry_empty ) {
    if ( !exists2( c_entry_empty ) ) {
      DSET_LOG( 3, spacer.spacer + "Need to poke '' entry." );
      poke( c_entry_empty );
    }
  }
  
  Utils::magic_ptr<Entry> old_e( m_subcontext->fetch2( entry, true ) );
  if( old_e ) {
    //&& fetch( entry )->exists( c_attr_entry_rtime ) ) {
    if ( Modtime( old_e->attr( c_attr_entry_rtime )->valuestr() )
         == Transaction::poke_modtime() ) {
      DSET_LOG( 1, spacer.spacer + "Entry has not changed, skipping." );
      return false;
    }
  }
  // This entry has formally changed. We need to update the real subcontext.
  
  // Pull out the child entry.
  Utils::magic_ptr<Entry> ce( m_overlay_subcontext->fetch2( entry, true ) );
  if( ce ) {
    if( ce->attr_isdefault( c_attr_entry ) ) ce.zap();
  }
  
  // Pull out the parent entry.
  Utils::magic_ptr < Entry > pe;
  {
    Utils::magic_ptr < Dataset > parent( dataset_inherit() );
    DSET_LOG( 2, spacer.spacer + "Checking inheritance." );
    if( parent ) {
      // Okay, now see if there's a matching entry.
      DSET_LOG( 2, spacer.spacer + "Got parent dataset." );
      parent->poke( entry ); // Uh-oh, don't do this, we end up with a big loop... [Try it now...]
      // No need for above, it'll happen anyway.
      pe = parent->fetch2( entry );
      if( pe ) {
        // Good, there is.
        DSET_LOG( 2, spacer.spacer + "Got parent entry." );
        if( pe->exists( c_attr_origin ) ) {
          // Origin tests.
          if( Path( pe->attr( c_attr_origin )->valuestr() )==m_trans_path.canonicalize() ) {
            throw std::string ("Origin of parent entry points to myself, suspected inheritance loop.");
          }
        } else {
          throw std::string ("No origin specified. Probable just cause for war.");
        }
      }
    }
  }
  
  if( old_e ) {
    std::string deps( old_e->attr( c_attr_entry_deps )->valuestr() );
    Modtime m( old_e->attr( c_attr_entry_rtime )->valuestr() );
    DSET_LOG( 1, "Deps mode is " + deps );
    if( deps == "both" ) {
      if(  ( ce && !( Modtime( ce->attr( c_attr_entry_rtime )->valuestr() ) > m ) )
           && ( pe && !( Modtime( pe->attr( c_attr_entry_rtime )->valuestr() ) > m ) ) ) {
        DSET_LOG( 1, spacer.spacer + "Entry has not changed, skipping. [MK1]" );
        return false;
      }
    } else if( deps == "self" ) {
      if( !pe && ( ce && !( Modtime( ce->attr( c_attr_entry_rtime )->valuestr() ) > m ) ) ) {
        DSET_LOG( 1, spacer.spacer + "Entry has not changed, skipping [MK2]" );
        return false;
      }
    } else if( deps == "parent" ) {
      if( !ce && ( pe && !( Modtime( pe->attr( c_attr_entry_rtime )->valuestr() ) > m ) ) ) {
        DSET_LOG( 1, spacer.spacer + "Entry has not changed, skipping [MK3]" );
        return false;
      }
    }
  } else {
    if( !ce && !pe ) {
      DSET_LOG( 1, spacer.spacer + "Nothing exists, nothing to do." );
      return false;
    }
  }
  
  m_poked.insert( entry );
  if( m_datastore.exists( m_trans_path ) ) {
    Transaction::add( m_trans_path );
  }
  
  Utils::magic_ptr < Entry > oe;
  bool vdset_stuff( false );
  if( pe ) {
    if( ce ) {
      // Copy and Merge.
      oe = new Entry();
      if( !ce->attr_isdefault( c_attr_entry ) ) {
        oe->merge( ce, false );
      }
      // Then merge.
      if( ce->attr_isnil( c_entry_empty ) ) {
        oe.zap();
      } else {
        oe->merge( pe, true );
        if( Modtime( pe->attr( c_attr_modtime )->valuestr() ) > Modtime( ce->attr( c_attr_modtime )->valuestr() ) ) {
          oe->attr( c_attr_modtime )->value( pe->attr( c_attr_modtime )->value() );
        }
        oe->add( new Attribute( c_attr_entry_deps, "both" ) );
        // Virtual dataset stuff?
        if( pe->exists( c_attr_subdataset ) ) {
          if( !ce->exists( c_attr_subdataset ) ) {
            vdset_stuff = true;
          }
        }
      }
    } else {
      oe = new Entry();
      oe->merge( pe, true );
      // Merge because we muck about with the rtime, and don't want to screw
      // things up in the parent.
      // Virtual dataset stuff?
      if( pe->exists( c_attr_subdataset ) ) {
        vdset_stuff = true;
      }
      oe->add( new Attribute( c_attr_entry_deps, "parent" ) );
    }
  } else if( ce ) {
    if( !ce->attr_isnil( c_entry_empty ) ) {
      oe = new Entry();
      oe->merge( ce, false );
      // Still merge, since we might have to change dtimes.
      oe->add( new Attribute( c_attr_entry_deps, "self" ) );
    }
  }
  
  // Strip out any DEFAULT/NIL attributes.
  if( oe ) {
    DSET_LOG( 1, spacer.spacer + "Stripping NIL/DEFAULT from final entry." );
    bool flag( true );
    do {
      flag = false;
      for( Entry::const_iterator i( oe->begin() ); i!=oe->end(); ++i ) {
        DSET_LOG( 1, spacer.spacer + "Scanning " + (*i).first.get() );
        if( oe->attr_isdefault( (*i).first ) || oe->attr_isnil( (*i).first ) ) {
          DSET_LOG( 1, spacer.spacer + "Found " + (*i).first.get() );
          oe->erase( (*i).first );
          flag = true;
          break;
        }
      }
    } while( flag );
  }
  DSET_LOG( 1, spacer.spacer + "Have completed entry" );
  
  if( vdset_stuff && m_datastore.exists( m_trans_path ) ) {
    DSET_LOG( 1, spacer.spacer + "Need to do virtual subdataset stuff." );
    if( !m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
      Utils::magic_ptr<Path> np;
      Utils::magic_ptr<Token::List> l( Utils::magic_cast<Token::List>( pe->attr( c_attr_subdataset )->value() ) );
      Utils::magic_ptr<Token::PList> out( new Token::PList );
      // Pass one. 
      bool added_dot( false );
      for( int i(0); i != l->length(); ++i ) {
        if( l->get( i ).toString().value()=="." ) {
          added_dot = true;
          out->add( new Token::String( "." ) );
          np = new Path( Path( fetch2( c_entry_empty, true )->attr( c_attr_dataset_inherit )->valuestr() + entry.get() ).canonicalize() );
          break;
        }
      }
      if( !added_dot ) {
        for( int i(0); i != l->length(); ++i ) {
          Path sp( m_trans_path + l->get(i).toString().value() );
          if( !sp.isRemote() ) {
            if( !np ) {
              np = new Path( sp.canonicalize() );
              added_dot = true;
              out->add( new Token::String( "." ) );
              break;
            }
          }
        }
      }
      if( added_dot ) {
        oe->attr( c_attr_subdataset )->value( magic_cast<Token::Token>( l ) );
        Path vdset( m_trans_path.asString() + entry.get() );
        if( !m_datastore.exists( vdset ) ) {
          DSET_LOG( 1, spacer.spacer + "Creating virtual dataset " + vdset.asString() + " inheriting from " + np->asString() );
          Utils::magic_ptr<Dataset> dset( m_datastore.create_virtual( vdset ) );
          DSET_LOG( 1, spacer.spacer + "Okay, created virtual. Now setting inheritance." );
          Utils::magic_ptr<Entry> e( new Entry );
          e->add( new Attribute( c_attr_dataset_inherit, np->asString() ) );
          dset->add2( e, c_entry_empty );
          dset->inherit_reset();
          DSET_LOG( 1, spacer.spacer + "Created virtual." );
        }
      }
    }
  }
  
  if( oe ) {
    for( Entry::const_iterator i( oe->begin() ); i!=oe->end(); ++i ) {
      if( oe->exists( (*i).first ) ) {
        if( (*i).second ) {
          if( (*i).second->value()->isPList() ) {
            for( int j(0); j<(*i).second->value()->toPList().length(); ++j ) {
              if( !(*i).second->value()->toPList().get(j).isString() ) {
                throw std::string( "Multivalued attribute " + (*i).first.get() + " contains non-string value: '" + (*i).second->value()->toPList().get(j).asString() + "'" );
              }
            }
          } else {
            if( !(*i).second->value()->isNil() && !(*i).second->value()->isString() ) {
              throw std::string( "Single valued attribute " + (*i).first.get() + " contains non-string value: '" + (*i).second->value()->asString() + "'" );
            }
          }
        }
      }
    }
  }
  
  if ( !oe ) {
    // This might happen if we've changed inheritance, and the entry doesn't
    // in either subcontext. Should probably be handled earlier.
    if ( entry.get().length() == 0 ) {
      // Don't think this should *ever* happen, except in a delete.
      // throw std::string( "Found no possible '' entry in " + m_trans_path.asString() );
    }
    if ( old_e ) {
      subcontext_pure()->remove( entry );
    }
    if( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
      m_datastore.erase( m_trans_path.asString() + entry.get() );
    }
  } else {
    // Set rtime, prevents us from poking the same thing twice.
    oe->add( Utils::magic_ptr<Attribute>( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) ) );
    oe->add( Utils::magic_ptr<Attribute>( new Attribute( c_attr_control, m_trans_path.canonicalize().asString() ) ) );
    if( ce ) {
      ce->add( Utils::magic_ptr<Attribute>( new Attribute( c_attr_control, m_trans_path.canonicalize().asString() ) ) );
    }
    if( !oe->exists( c_attr_origin ) ) {
      oe->add( Utils::magic_ptr<Attribute>( new Attribute( c_attr_origin, m_trans_path.canonicalize().asString() ) ) );
    }
    
    if ( !oe->exists( c_attr_entry ) || oe->attr( c_attr_entry )->value()->isNil() ) {
      // Our rename, or our delete?
      // Most likely the entry exists in the parent, but is deleted here.
      if ( entry.get().length() == 0 ) {
        // Don't think this should *ever* happen outside of the dataset being deleted.
        throw std::string( "Found attempt to delete '' entry in " + m_trans_path.asString() );
      }
      //if ( m_subcontext->exists( entry ) ) {
      //        m_subcontext->remove( entry );
      //}
      
      if( !oe->exists( c_attr_entry_dtime ) ) {
        oe->add( new Attribute( c_attr_entry_dtime, Transaction::poked().asString() ) );
      }
      if( m_datastore.exists( m_trans_path.asString()+entry.get() ) ) {
        m_datastore.erase( m_trans_path.asString()+entry.get() );
      }
      subcontext_pure()->add( entry, oe );
    } else {
      if( oe->exists( c_attr_entry_dtime ) ) {
        oe->erase( c_attr_entry_dtime );
      }
      if( entry != c_entry_empty ) {
        bool sdset( false );
        if( oe->exists( c_attr_subdataset ) ) {
          for( int i(0); i != oe->attr( c_attr_subdataset )->value()->toList().length(); ++i ) {
            if( "."==oe->attr( c_attr_subdataset )->value()->toList().get( i ).toString().value() ) {
              sdset = true;
              break;
            }
          }
        }
        if( !sdset ) {
          DSET_LOG( 1, spacer.spacer + "Considering erasing path here..." );
          if( m_datastore.exists( m_trans_path.asString()+entry.get() ) ) {
            m_datastore.erase( m_trans_path.asString()+entry.get() );
          }
        }
      }
      subcontext_pure()->add( entry, oe );
      // This will actually set the MODTIME wrong.
      // But that's okay - since we're only poking when either a STORE (or, more
      // generally, an add2) has taken place, or else when the inheritance has
      // changed, then our MODTIME is more useful than that of the standard.
      // This has been discussed on the ACAP mailing list, so I'm not making
      // this up, honest!
    }
  }
  
  TRANSACTION_VALIDATE();
  Transaction::add( subcontext_pure() );
  
  // And now update our children.
  Utils::magic_ptr<Entry> empty_e( m_subcontext->fetch2( c_entry_empty, true ) );
  if( empty_e ) {
    if( empty_e->exists( c_attr_dataset_children ) ) {
      Utils::magic_ptr<Token::PList> pl( magic_cast<Token::PList>( empty_e->attr( c_attr_dataset_children )->value() ) );
      for( int i( 0 ); i < pl->length(); ++i ) {
        Path child_path( pl->toList().get( i ).toString().value() );
        child_path = child_path.canonicalize();
        if( oe ) {
          if( Path( oe->attr( c_attr_origin )->valuestr() ).canonicalize()==child_path ) {
            throw std::string( "Child appears to be origin, suspected inheritance loop." );
          }
        }
        if( m_datastore.exists( child_path ) ) {
          Utils::magic_ptr<Dataset> child( m_datastore.dataset( child_path ) );
          //if( child->exists( entry ) ) {
          //  child->fetch( entry )->store_default( c_attr_entry_rtime );
          //}
          child->poke( entry );
        } // If it doesn't exist, it will do soon.
      }
    }
  }
  
  return true;
}

void Dataset::add2( magic_ptr<Entry> const & e, Utils::StringRep::entry_type const & entry, bool replace ) {
  Spacer spacer( '#' );
  TRANSACTION_VALIDATE();
  Transaction::add( m_requested_subcontext );
  // Validity checks.
  Utils::magic_ptr<Entry> requested_empty( m_requested_subcontext->fetch2( c_entry_empty, true ) );
  {
    if( entry.get().find( "/" )!=std::string::npos ) {
      throw Data::Exceptions::invalid_attr( "Validation error: entry name supplied contains illegal characters." );
    }
    if( entry.get().length() && entry.get()[0]=='.' ) {
      throw Data::Exceptions::invalid_attr( "Validation error: entry name supplied contains illegal characters." );
    }
    
    // UTF-8 validation.
    try {
      Utf8::validate( entry.get(), true );
    } catch( std::exception const & e ) {
      throw Data::Exceptions::invalid_attr( e.what() );
    }
    
    // Check attributes.
    for( Entry::const_iterator i( e->begin() ); i!=e->end(); ++i ) {
      if( (*i).first.get().find_first_of( "%*" )!=std::string::npos ) {
        throw Data::Exceptions::invalid_attr( "Validation error: attribute name contains illegal characters." );
      }
      try {
        Utf8::validate( (*i).first.get(), true );
      } catch( std::runtime_error const & e ) {
        throw Data::Exceptions::invalid_attr( e.what() );
      }
      if( !(*i).second->value()->isNil() ) {
        // Generic first.
        if( (*i).second->value()->isPList() ) {
          for( int j( 0 ); j!=(*i).second->value()->toPList().length(); ++j ) {
            if( !(*i).second->value()->toPList().get(j).isString() ) {
              throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), (*i).first.get(), "Validation error: List member is not String." );
            }
          }
        } else if( !(*i).second->value()->isString() ) {
          throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), (*i).first.get(), "Validation error: value is not String or List." );
        }
        
        if( (*i).first==c_attr_entry ) {
          if( (*i).second->value()->toString().value().find_first_of( std::string( "/\0", 2 ) )!=std::string::npos ) {
            throw Data::Exceptions::invalid_data_sys( m_trans_path.asString()+entry.get(), "entry", "Validation error: new entry name contains illegal characters" );
          }
          if( (*i).second->value()->toString().value().length()
              && (*i).second->value()->toString().value()[0]=='.' ) {
            throw Data::Exceptions::invalid_data_sys( m_trans_path.asString()+entry.get(), "entry", "Validation error: new entry name starts with dot." );
          }
          try {
            Utf8::validate( (*i).second->value()->toString().value(), true );
          } catch( std::runtime_error const & e ) {
            throw Data::Exceptions::invalid_data_sys( m_trans_path.asString()+entry.get(), "entry", e.what() );
          }
        } else if( (*i).first==c_attr_subdataset ) {
          if( (*i).second->value()->toList().length()==0 ) {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), "subdataset", "Validation error: zero-length list in subdataset." );
          }
          for( int x( 0 ); x!=(*i).second->value()->toList().length(); ++x ) {
            std::string const & y( (*i).second->value()->toList().get(x).toString().value() );
            try {
              Utf8::validate( y, true );
            } catch( std::runtime_error const & e ) {
              throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), "subdataset", e.what() );
            }
            if( y.length()==0 ) {
              throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), "subdataset", "Zero length subdataset URL." );
            }
          }
        } else if( (*i).first==c_attr_modtime ) {
          if( (*i).second->value()->toString().value().find_first_not_of( "1234567890" )!=std::string::npos ) {
            throw Data::Exceptions::invalid_data( m_trans_path.asString()+entry.get(), "modtime", "Validation error: modtime supplied is not numeric." );
          }
        }
      }
    }
  }
  // Checks on the delta:
  if ( entry->empty() ) {
    
    if ( e->exists_pure( c_attr_entry ) ) {
      if ( e->attr_isdefault( c_attr_entry ) ) {
        // DEFAULT
        m_datastore.erase( m_trans_path, false );
        return ;
      } else if( e->attr_isnil( c_attr_entry ) ) {
        // NIL
        m_datastore.erase( m_trans_path );
        return ;
      } else if ( e->attr( c_attr_entry )->valuestr() != c_entry_empty.get() ) {
        // *
        throw std::string( "Cannot rename dataset entry." );
      }
    }
    
    if ( e->exists_pure( c_attr_subdataset ) ) {
      throw std::string( "Attribute subdataset is meaningless within dataset entry." );
    }
    
    if ( e->exists_pure( c_attr_dataset_inherit ) ) {
      if ( e->attr_isdefault( c_attr_dataset_inherit ) ) {
        // DEFAULT
        DSET_LOG( 1, spacer.spacer + "dataset.inherit - DEFAULT" );
        Path byowner_ppath( m_trans_path.translate( Path::ByOwner ).parent() );
        Utils::magic_ptr<Dataset> dset( m_datastore.dataset( byowner_ppath ) );
        Utils::magic_ptr<Entry> parent_empty( dset->fetch2( c_entry_empty ) );
        if( parent_empty->exists( c_attr_dataset_inherit ) ) {
          inherit_set( Path( parent_empty->attr( c_attr_dataset_inherit )->valuestr() ).asString() + m_trans_path.mybit().get() );
        }
      } else if( e->attr_isnil( c_attr_dataset_inherit ) ) {
        // NIL
        DSET_LOG( 1, spacer.spacer + "dataset.inherit - NIL" );
        inherit_clear();
      } else {
        // *
        DSET_LOG( 1, spacer.spacer + "dataset.inherit - SET" );
        inherit_set( e->attr( c_attr_dataset_inherit )->valuestr() );
      }
    }
    
    if ( e->exists_pure( c_attr_dataset_acl ) ) {
      if ( e->attr_isdefault( c_attr_dataset_acl ) || e->attr_isnil( c_attr_dataset_acl ) ) {
        // NIL
        throw std::string( "Cannot delete dataset.acl attribute." );
      } else {
        // *
        if( !e->attr( c_attr_dataset_acl )->value_acl() ) {
          throw std::string( "No ACL value for dataset.acl?" );
        }
      }
    } else if( replace ) { // Ah... Force addition.
      DSET_LOG( 1, "Re-adding dataset.acl during load (old dump?)" );
      if( requested_empty && requested_empty->exists( c_attr_dataset_acl ) ) {
        e->add( requested_empty->attr( c_attr_dataset_acl ) );
      }
    }
    
    // If dataset.acl, or dataset.acl.*, have changed, then we need to repoke everything. (Actually, only our stuff ** TODO.)
    for( Entry::const_iterator i( e->begin() ); i!=e->end(); ++i ) {
      if( (*i).first->find( "dataset.acl" )==0 ) {
        Utils::magic_ptr<Attribute> a;
        if( requested_empty && requested_empty->exists_pure( (*i).first ) ) {
          a = requested_empty->attr( (*i).first );
        }
        if( e->attr_isdefault( (*i).first ) ) {
          if( a ) {
            m_inherit_changed = true;
          }
        } else if( e->attr_isnil( (*i).first ) ) {
          if( !a || a->value() ) {
            m_inherit_changed = true;
          }
        } else {
          if( !(*i).second->value_acl() ) {
            throw std::runtime_error( "ACL attribute " + (*i).first.get() + " has no value_acl!" );
          }
          if( a && a->value() ) {
            DSET_LOG( 1, "Comparing ACL values." );
            m_inherit_changed = true;
            if( *a->value_acl() == *(*i).second->value_acl() ) {
              m_inherit_changed = false;
            }
            DSET_LOG( 1, "Done." );
          }
        }
      }
    }
    
  } else { // Other entries.
    if ( e->exists_pure( c_attr_entry ) ) {
      DSET_LOG( 1, spacer.spacer + "Entry " + entry.get() + " has altered entry attribute." );
      m_deletion = true;
      if ( e->attr_isdefault( c_attr_entry ) ) {
        DSET_LOG( 1, spacer.spacer + "Delta is DEFAULT." );
        // DEFAULT.
        if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
          m_datastore.erase( m_trans_path.asString() + entry.get(), false );
        }
        //for( Entry::const_iterator i( e->begin() ); i!=e->end(); ++i ) {
        //      e->erase( (*i).first );
        //}
      } else if( e->attr_isnil( c_attr_entry ) ) {
        DSET_LOG( 1, spacer.spacer + "Delta is NIL." );
        // NIL
        if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
          m_datastore.erase( m_trans_path.asString() + entry.get() );
        }
        //for( Entry::const_iterator i( e->begin() ); i!=e->end(); ++i ) {
        //      e->erase( (*i).first );
        //}
      } else if( e->attr( c_attr_entry )->valuestr() != entry.get() ) {
        DSET_LOG( 1, spacer.spacer + "Delta is renaming to '" + e->attr( c_attr_entry )->value()->asString() + "'" );
        // *
        if ( entry.get() != e->attr( c_attr_entry )->valuestr() ) {
          if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
            m_datastore.rename( m_trans_path.asString() + entry.get(),
                                m_trans_path.asString() + e->attr( c_attr_entry )->valuestr() );
          }
        }
      }
    }
    
    if ( e->exists_pure( c_attr_subdataset ) ) {
      if ( e->attr_isdefault( c_attr_subdataset ) ) {
        // DEFAULT
        if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
          m_datastore.erase( m_trans_path.asString() + entry.get(), false );
        }
      } else if( e->attr_isnil( c_attr_subdataset ) ) {
        // NIL
        if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
          m_datastore.erase( m_trans_path.asString() + entry.get() );
        }
      } else {
        // *
        Utils::magic_ptr<Comparators::Comparator> comp( Comparators::Comparator::comparator( "i;octet" ) );
        if ( comp->equals( e->attr( c_attr_subdataset ), magic_ptr<Attribute>( new Attribute( c_entry_empty, magic_ptr<Token::Token>( new Token::String( "." ) ) ) ) ) ) {
          // Containing "."
          if ( !m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
            m_datastore.create( m_trans_path.asString() + entry.get() );
          }
        } else {
          if ( m_datastore.exists( m_trans_path.asString() + entry.get() ) ) {
            m_datastore.erase( m_trans_path.asString() + entry.get() );
          }
        }
      }
    }
  }
  // Merge in existing values.
  e->add( new Attribute( c_attr_entry_rtime, Transaction::poked().asString() ) );
  if ( !e->exists_pure( c_attr_entry ) || !e->attr( c_attr_entry )->value()->isNil() ) {
    Utils::magic_ptr<Entry> rq( m_requested_subcontext->fetch2( entry, true ) );
    // Wasn't specified or isn't NIL/DEFAULT if it was.
    DSET_LOG( 1, spacer.spacer + "Merging values." );
    if( rq ) {
      // Quite possibly setting an ACL. Let's be certain:
      m_deletion = true;
    }
    if ( rq && !replace ) { // Never replace empty entry.
      for( Entry::const_iterator ei( rq->begin() );
           ei!=rq->end(); ++ei ) {
        if( !e->exists_pure( (*ei).first ) ) {
          DSET_LOG( 1, spacer.spacer + "Copying attribute " + (*ei).first.get() );
          e->add( (*ei).second );
        }
      }
      if( !e->exists_pure( c_attr_entry ) || e->attr( c_attr_entry )->value()->isNil() ) {
        e->add( rq->attr( c_attr_entry )->clone() );
        e->attr( c_attr_entry )->value( Utils::magic_ptr<Token::Token>( new Token::String( entry.get() ) ) );
      }
      if ( entry.get() != e->attr( c_attr_entry )->valuestr() ) {
        m_deletion = true;
        m_requested_subcontext->remove( entry );
      }
    } else {
      // Add a suitable default entry attribute.
      e->add( new Attribute( c_attr_entry, entry.get() ) );
    }
    m_requested_subcontext->add( e->attr( c_attr_entry )->valuestr(), e );
  } else {
    DSET_LOG( 1, spacer.spacer + "Erasing entry " + entry.get() );
    if ( e->attr_isdefault( c_attr_entry ) ) {
      m_requested_subcontext->remove( entry );
    } else {
      Utils::magic_ptr<Entry> marker( new Entry( entry ) );
      marker->store_nil( c_attr_entry );
      m_requested_subcontext->add( entry, marker );
    }
  }
  
  DSET_LOG( 2, spacer.spacer + "Poking entry " + entry.get() );
  poke( entry );
  if ( e->exists( c_attr_entry ) && !e->attr( c_attr_entry )->value()->isNil() ) {
    if ( e->attr( c_attr_entry )->valuestr() != entry.get() ) {
      DSET_LOG( 2, spacer.spacer + "Poking rename target entry" );
      poke( e->attr( c_attr_entry )->valuestr() );
    }
  }
  DSET_LOG( 2, spacer.spacer + "Pokes complete for " + entry.get() );
  //if( entry == c_entry_empty ) {
  //  m_inherit_changed = true;
  //}
  inherit_recalculate();
  DSET_LOG( 2, spacer.spacer + "Add2 completed." );
}

// Called after an add2 involving dataset.inherit, or by the parent Dataset when something major happens.

void Dataset::inherit_recalculate() {
  if( m_inherit_changed ) {
    m_deletion = true;
    Master::master()->log( 5, "Inheritance has changed, recalculating." );
    // Repoke everything so far.
    t_poked tmp( m_poked );
    // Now repoke everything we have.
    {
      for( const_iterator i( begin() );
           i!=end(); ++i ) {
        tmp.insert( (*i).second );
      }
    }
    // And everything our new parent has, if we have one.
    {
      Utils::magic_ptr<Entry> empty_e( m_subcontext->fetch2( c_entry_empty, true ) );
      if( empty_e ) {
        if( empty_e->exists( c_attr_dataset_inherit ) ) {
          if( m_datastore.exists( empty_e->attr( c_attr_dataset_inherit )->valuestr() ) ) {
            DSET_LOG( 2, "Adding parent entries." );
            Utils::magic_ptr<Dataset> dset( m_datastore.dataset( empty_e->attr( c_attr_dataset_inherit )->valuestr() ) );
            for( Dataset::const_iterator i( dset->begin() );
                 i!=dset->end(); ++i ) {
              tmp.insert( (*i).second );
            }
            for( t_poked::const_iterator i( dset->m_poked.begin() );
                 i!=dset->m_poked.end(); ++i ) {
              tmp.insert( (*i) );
            }
          }
        }
      }
    }
    
    subcontext_pure()->clear();
    subcontext_pure(true)->clear();
    
    for( t_poked::const_iterator i( tmp.begin() );
         i!=tmp.end(); ++i ) {
      DSET_LOG( 1, "Poking in " + m_trans_path.asString() + " for " + (*i).get() );
      poke( *i );
    }
    m_inherit_changed = false;
    DSET_LOG( 5, "Done inheritance recalc." );
  }
}

/*
void Dataset::register_context( ContextPtr const & c ) {
  Threading::Lock l__inst( m_cxt_lock );
  m_contexts.insert( c );
}

void Dataset::unregister_context( ContextPtr const & c ) {
  Threading::Lock l__inst( m_cxt_lock );
  m_contexts.erase( c );
}
*/

void Dataset::path( Path const & p ) {
  TRANSACTION_VALIDATE();
  m_trans_path = p;
  inherit_reset();
}

Path const & Dataset::path() const {
  if ( Transaction::mine() ) {
    return m_trans_path;
  } else {
    return m_path;
  }
}

bool Dataset::erase() {
  Utils::magic_ptr<Entry> empty( fetch2( c_entry_empty, true ) );
  if ( empty ) {
    DSET_LOG( 1, "+ Erasing " + m_trans_path.asString() );
    // First: If we're inheriting, stop that by simulating a STORE:
    if( empty->exists( c_attr_dataset_inherit ) ) {
      DSET_LOG( 1, "+ Dropping inheritance." );
      Utils::magic_ptr<Entry> e( new Entry );
      e->store_default( c_attr_dataset_inherit );
      add2( e, c_entry_empty );
    }
    DSET_LOG( 1, "+ Stripping empty entry." );
    m_requested_subcontext->remove( c_entry_empty );
    m_overlay_subcontext->remove( c_entry_empty );
    m_subcontext->remove( c_entry_empty );
    DSET_LOG( 1, "+ Teardown of data." );
    for( Subcontext::const_iterator i( m_requested_subcontext->begin() ); i!=m_requested_subcontext->end(); ++i ) {
      if( (*i).second!=c_entry_empty ) {
        Utils::magic_ptr<Entry> e( new Entry );
        e->store_default( c_attr_entry );
        add2( e, (*i).second );
      }
    }
    DSET_LOG( 1, "+ Teardown of last pokes." );
    for( t_poked::const_iterator i( m_poked.begin() ); i!=m_poked.end(); ++i ) {
      if( (*i)!=c_entry_empty ) {
        Utils::magic_ptr<Entry> e( new Entry );
        e->store_default( c_attr_entry );
        add2( e, (*i) );
      }      
    }
    DSET_LOG( 1, "+ Final clear" );
    // Okay, everything now zapped, so remove the empty entry - manually, as we have to, along with anything else remaining.
    m_requested_subcontext->clear();
    m_overlay_subcontext->clear();
    m_subcontext->clear();
    DSET_LOG( 1, "+ Done." );
  }
  return true;
}

Infotrope::Utils::magic_ptr<Subcontext> & Dataset::subcontext_pure( bool noinherit ) {
  if( noinherit ) {
    return m_overlay_subcontext;
  } else {
    return m_subcontext;
  }
}

Dataset::iterator Dataset::begin( bool noinherit ) {
  return subcontext_pure( noinherit )->begin();
}
Dataset::iterator Dataset::end( bool noinherit ) {
  return subcontext_pure( noinherit )->end();
}

Dataset::const_iterator Dataset::begin( bool noinherit ) const {
  if( noinherit ) {
    return m_overlay_subcontext->begin();
  } else {
    return m_subcontext->begin();
  }
}
Dataset::const_iterator Dataset::end( bool noinherit ) const {
  if( noinherit ) {
    return m_overlay_subcontext->end();
  } else {
    return m_subcontext->end();
  }
}

Infotrope::Utils::magic_ptr<Entry> const & Dataset::fetch2( Infotrope::Utils::StringRep::entry_type const & e, bool nih ) const {
  if( e==c_entry_empty && !Transaction::mine() ) {
    return nih ? m_overlay_empty : m_empty;
  }
  if( nih ) {
    return m_overlay_subcontext->fetch2( e, true );
  } else {
    return m_subcontext->fetch2( e, true );
  }
}
bool Dataset::exists2( Infotrope::Utils::StringRep::entry_type const & e, bool nih ) const {
  if( e==c_entry_empty && !Transaction::mine() ) {
    return nih ? m_overlay_empty : m_empty;
  }
  if( nih ) {
    return m_overlay_subcontext->exists2( e );
  } else {
    return m_subcontext->exists2( e );
  }
}
