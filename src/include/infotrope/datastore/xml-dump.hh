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
#ifndef ACAP_XML_DUMP_HH
#define ACAP_XML_DUMP_HH

#include <fstream>
#include <string>

namespace Infotrope {
  
  namespace Data {
    class Subcontext;
    class Entry;
    
    class Dump {
    public:
      Dump();
      virtual ~Dump();
      
      void dump_spacer();
      void open( std::string const & filename );
      void dump_sc( Subcontext const & s );
      void dump_entry( std::string const & n, Entry const & e );
      void dump();
      void dump( std::string const & dset );
      std::string entity_encode( std::string const & si, char c, std::string const & rep );
      std::string dump_value( std::string const & val );
      void dump_value_xml( std::string const & val );
      void dumper( bool loop=false, bool trans=false );
      
    protected:
      virtual void dump_main();
      std::ofstream m_xml;
      int m_spacer;
    };
    
  }
}

#endif
