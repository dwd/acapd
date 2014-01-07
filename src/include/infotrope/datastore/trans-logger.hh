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
 *  Copyright  2003-2004 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/

#ifndef INFOTROPE_DATASTORE_TRANSLOG_HH
#define INFOTROPE_DATASTORE_TRANSLOG_HH

#include <infotrope/datastore/notify-sink.hh>
#include <infotrope/datastore/xml-dump.hh>

namespace Infotrope {
  namespace Data {
    class TransLogger : public Infotrope::Notify::Sink, public Infotrope::Data::Dump {
    public:
      TransLogger();
      ~TransLogger();
      
      virtual void handle_notify( Utils::magic_ptr<Notify::Source> const &, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which ) throw();
      virtual void dump_main() {
	process_all();
      }
    };
  }
}

#endif
