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

#ifndef INFOTROPE_AUTHZ_HH
#define INFOTROPE_AUTHZ_HH

#include <infotrope/utils/stringrep.hh>
#include <infotrope/threading/lock.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <map>

namespace Infotrope {
  namespace Data {
    
    class AuthZ {
    public:
      AuthZ( Utils::StringRep::entry_type const & userid );
      virtual ~AuthZ();
      enum member_type {
	NotMember = 0,
	Everybody = 1,
	InGroup = 2,
	Equivalent = 3
      }; // These are chosen to match Cyrus IMAPd.
      member_type memberOf( Utils::StringRep::entry_type const & id );
      member_type negativeMemberOf( Utils::StringRep::entry_type const & id );
      
      static Utils::magic_ptr<AuthZ> authz( Utils::StringRep::entry_type const & userid );
      
    protected:
      virtual member_type get_memberOf( Utils::StringRep::entry_type const & id );
      virtual member_type get_negativeMemberOf( Utils::StringRep::entry_type const & id );
      
      void invalidate_cache( Utils::StringRep::entry_type const & id );
      void invalidate_cache();
    protected:
      Utils::StringRep::entry_type const m_userid;
      Utils::StringRep::entry_type const m_neg_userid;
      Utils::StringRep::entry_type const m_realm;
    private:
      typedef std::map< Utils::StringRep::entry_type, member_type, Utils::StringRep::entry_type_less > t_group_cache;
      t_group_cache m_pve;
      t_group_cache m_nve;
      Threading::Mutex m_mutex;
    };
    
  }
  
}

#endif
