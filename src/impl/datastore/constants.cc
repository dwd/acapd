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
#include <infotrope/datastore/constants.hh>

namespace Infotrope {
  
  namespace Constants {
    // vendor.infotrope attribute naming system as follows:
    // First, "vendor.infotrope", meaning it's one of ours.
    // Next, "system", meaning it's not allowed to be set in a STORE, or
    //       "ext", meaning it can be set in a STORE.
    // Next, "noinherit", meaning it won't be inherited, or
    //       "inherit", meaning it will be.
    //       Only inherited attributes will end up in a SEARCH or CONTEXT
    //       subcontext, so use with care.
    // Next, "dataset", meaning it only exists in the "" entry of a dataset, or
    //       "entry", meaning it applies to any entry.
    
    // "system" attributes are not returned unless specifically asked for.
    // This is largely because they don't concern "normal" clients, and
    // many existing clients ask for all attributes using "*".
    
    Utils::StringRep::entry_type const c_attr_dataset_children( "vendor.infotrope.system.noinherit.dataset.children" );
    Utils::StringRep::entry_type const c_attr_dataset_siblings( "vendor.infotrope.system.noinherit.dataset.siblings" );
    Utils::StringRep::entry_type const c_attr_dataset_ctime( "vendor.infotrope.system.noinherit.dataset.create-modtime" );
    Utils::StringRep::entry_type const c_attr_dataset_inherit_modtime( "vendor.infotrope.system.noinherit.dataset.inherit-modtime" );
    Utils::StringRep::entry_type const c_attr_dataset_type( "vendor.infotrope.system.noinherit.dataset.type" );
    Utils::StringRep::entry_type const c_attr_dataset_inherit( "dataset.inherit" );
    Utils::StringRep::entry_type const c_attr_dataset_acl( "dataset.acl" );
    Utils::StringRep::entry_type const c_attr_subdataset( "subdataset" );
    Utils::StringRep::entry_type const c_attr_modtime( "modtime" );
    Utils::StringRep::entry_type const c_entry_empty( "" );
    Utils::StringRep::entry_type const c_attr_entry_rtime( "vendor.infotrope.system.inherit.entry.reconstruct-modtime" );
    Utils::StringRep::entry_type const c_attr_entry_occlude( "vendor.infotrope.system.noinherit.entry.original-name" );
    Utils::StringRep::entry_type const c_attr_entry_deps( "vendor.infotrope.system.noinherit.entry.dependencies" );
    Utils::StringRep::entry_type const c_attr_origin( "vendor.infotrope.system.inherit.entry.origin" );
    Utils::StringRep::entry_type const c_attr_control( "vendor.infotrope.system.inherit.entry.control" );
    // Actually, control is overwritten during inheritance, but since a SEARCH "inherits",
    // we need to keep this for later ACL checking.
    Utils::StringRep::entry_type const c_attr_entry_dtime( "vendor.infotrope.system.inherit.entry.delete-modtime" );
    // Although inherited, it will be stripped from final subcontext entries,
    // if the final subcontext contains a non-NIL entry attribute.
    Utils::StringRep::entry_type const c_attr_entry( "entry" );
    
    Utils::StringRep::entry_type const c_authid_owner( "$owner" );
    Utils::StringRep::entry_type const c_authid_not_owner( "-$owner" );
    Utils::StringRep::entry_type const c_authid_admin( "admin" );
    Utils::StringRep::entry_type const c_authid_anyone( "anyone" );
    Utils::StringRep::entry_type const c_authid_not_anyone( "-anyone" );
    
    
    // Option entries all begin with vendor.infotrope, and are all held within
    // /option/.../vendor.infotrope/
    Utils::StringRep::entry_type const c_option_datadir( "vendor.infotrope.datadir" );
    Utils::StringRep::entry_type const c_option_cachedir( "vendor.infotrope.datadir" );
  }
  
}
