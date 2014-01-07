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
#ifndef INFOTROPE_DATASTORE_CONSTANTS
#define INFOTROPE_DATASTORE_CONSTANTS

#include <infotrope/utils/stringrep.hh>

namespace Infotrope {
    
    namespace Constants {
      // The name of the attribute which holds the list of the datasets
      // which directly inherit from this dataset.
      // Attribute should only be in "".
      // This attribute is not inherited - doing so would lead to Weirdness.
      extern Utils::StringRep::entry_type const c_attr_dataset_children;
      
      // The name of the attribute which holds the type of the dataset.
      // Attribute should only be in "".
      // This attribute is not inherited - doing so would lead to Weirdness.
      // Used only for "virtual".
      extern Utils::StringRep::entry_type const c_attr_dataset_type;
      
      // The name of the attribute which holds the list of the datasets
      // which share subcontexts with this one.
      // Attribute should only be in "".
      // This attribute MUST NOT be inherited.
      // Unused.
      extern Utils::StringRep::entry_type const c_attr_dataset_siblings;
      
      // The name of the attribute which holds the path of the dataset
      // which is really referred to in subdataset (".").
      // Used for virtual subdatasets.
      // This attribute MUST NOT be inherited.
      // Unused, but should be.
      extern Utils::StringRep::entry_type const c_attr_realpath;
      
      // The name of the attribute which holds the modification time of the
      // c_attr_dataset_modtime attribute.
      // Attribute should only be in "".
      // Unused.
      extern Utils::StringRep::entry_type const c_attr_dataset_inherit_modtime;
      
      // The name of the attribute which holds the last MODTIME of when
      // this entry was poked (data reconstructed from the overlay and the
      // parent dataset).
      extern Utils::StringRep::entry_type const c_attr_entry_rtime;
      
      // When the entry was deleted. Only actually set on marker entries.
      // That's entries where the "entry" attribute is set to NIL or
      // DEFAULT.
      // This attribute is removed from final subcontext entries, but is
      // inherited between datasets.
      extern Utils::StringRep::entry_type const c_attr_entry_dtime;
      
      // What the entry (final) was made from.
      // Either self, parent, or both.
      extern Utils::StringRep::entry_type const c_attr_entry_deps;
      
      // The name of the attribute which holds the local path to the
      // dataset in which this entry is originally defined. This is
      // used to prevent inheritance loops, and is defined by poke
      // when it encounters an entry without it.
      // Poke throws an exception if it encounters a parent entry marked
      // with an origin of itself.
      extern Utils::StringRep::entry_type const c_attr_origin;
      
      // The name of the attribute which hold the local path to the
      // dataset in which this entry actually belongs. This is used to
      // handle ACL checks.
      // Poke sets this on every entry is processes.
      extern Utils::StringRep::entry_type const c_attr_control;
      
      // The authorization identifier with the special meaning of
      // the owner of the current dataset, "$owner".
      extern Utils::StringRep::entry_type const c_authid_owner;
      
      // The authorization identifier with the special meaning of
      // the negative owner, that is, rights assigned to this user will
      // be stripped from the owner of the dataset. Represented as "-$owner".
      extern Utils::StringRep::entry_type const c_authid_not_owner;
      
      // The authorization identifier with the special ability to bypass
      // ACL right calculation.
      extern Utils::StringRep::entry_type const c_authid_admin;
      
      // The authorization identifier which maps to all users.
      extern Utils::StringRep::entry_type const c_authid_anyone;
      
      // And the authzid which maps to all negative users.
      // More accurately, rights assigned to this user will be stripped
      // from everyone.
      extern Utils::StringRep::entry_type const c_authid_not_anyone;
      
      // The name of the attribute which holds the name of the dataset
      // for which this dataset inherits, in the entry "".
      extern Utils::StringRep::entry_type const c_attr_dataset_inherit;
      
      // The name of the attribute which holds the master fallback ACL for the
      // dataset.
      extern Utils::StringRep::entry_type const c_attr_dataset_acl;
      
      // The name of the attribute which hold a list of subdatasets of the
      // entry.
      extern Utils::StringRep::entry_type const c_attr_subdataset;
      
      // The name of the entry which hold the last modtime when this entry
      // was formally modified.
      extern Utils::StringRep::entry_type const c_attr_modtime;
      
      // The name of the entry which holds information corresponding to the
      // dataset as a whole.
      extern Utils::StringRep::entry_type const c_entry_empty;
      
      // The name of the attribute which tracks renames of entries.
      // Currently largely unused.
      extern Utils::StringRep::entry_type const c_attr_entry_occlude;
      
      // The name of the attribute holding the entry's name.
      extern Utils::StringRep::entry_type const c_attr_entry;
      
      // The name of the option holding the current persistent data directory, usually
      // /var/lib/acap/
      extern Utils::StringRep::entry_type const c_option_datadir;
      
      // The name of the option holding the current cache data directory, usually
      // /var/cache/acap/
      extern Utils::StringRep::entry_type const c_option_cachedir;
      
    }
}

#endif
