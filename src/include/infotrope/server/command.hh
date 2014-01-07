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
 *  Copyright  2003 Dave Cridland
 *  dave@cridland.net
 ****************************************************************************/

#ifndef INFOTROPE_SERVER_COMMAND_HH
#define INFOTROPE_SERVER_COMMAND_HH

#include <infotrope/server/worker.hh>
#include <infotrope/utils/istring.hh>

namespace Infotrope {
  namespace Server {
    
    class Command : public Infotrope::Threading::Thread {
    public:
      Command( bool sync, Worker & worker );
      virtual ~Command();
      
      // Override this.
      virtual void internal_parse( bool ) = 0;
      bool parse( bool );
      
      virtual void * runtime();
      virtual void main() = 0;
      bool feed();
      virtual bool feed_internal();
      void feed_me();
      void execute();
      bool dead() {
	return m_dead;
      }
      
      class RegisterBase {
      protected:
	RegisterBase( Infotrope::Utils::istring const & cmd, Infotrope::Server::Scope s );
      public:
	virtual Command * create( Worker & worker ) const = 0;
	virtual ~RegisterBase() {}
      };
      template< typename Tp > class Register : public RegisterBase {
      public:
	Register( Infotrope::Utils::istring const & cmd, Infotrope::Server::Scope s ) : RegisterBase( cmd, s ) {
	}
	virtual Command * create( Worker & worker ) const {
	  return new Tp( worker );
	}
      };
      static Command * create( Utils::istring const & command, Infotrope::Server::Scope s, Worker & worker );
      
    private:
      bool m_sync;
      bool m_dead;
      bool m_feeding;
    protected:
      Infotrope::Utils::magic_ptr<Infotrope::Token::List> m_toks;
      Worker & m_worker;
    };
  }
}

#endif
