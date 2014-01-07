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


#ifndef INFOTROPE_SERVER_MASTER_HH
#define INFOTROPE_SERVER_MASTER_HH

#include <map>
#include <deque>
#include <infotrope/threading/lock.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/config.h>
#include <sasl/sasl.h>

namespace Infotrope {
  namespace Data {
    class Datastore;
  }
  namespace Server {
    /*
      Master is responsible for handling incoming connections, and passing them onto
      Workers, and reaping Workers when they complete.
      The Master communicates with a Driver process, which actually handles listening to ports.
      Master also handles SASL setup and logging.
    */
    class Worker;
#ifdef INFOTROPE_ENABLE_XPERSIST
    class Session;
#endif
    class Master {
    public:
      Master( int ctrlfd, std::string const & datadir );
      ~Master();
      int run();
      
      // Send 'L' message, with priority 1-9.
      void log( int pri, std::string const & ) const;
      
      // Send 'E' message - server will not restart after this.
      void log_except( std::string const & ) const;
      
      // Access worker threads.
      bool exists( int ) const;
      Infotrope::Utils::magic_ptr<Worker> get( int ) const;
      
      static Master * master();
      
      // dead is sent when a Worker has been told to shutdown, and has completed running all pending commands.
      void dead( int );
      
      // We do this if we generate an exception.
      // All workers are rapidly shutdown, and * BYE is unilaterally sent.
      void shutdown_fast();
      
      // Backgrounded dump request.
      void full_dump();
      
      // Cyrus SASL support functions.
      std::string sasl_getopt( std::string const & path, std::string const & option );
      std::string sasl_log( int level, std::string const & msg );
      std::string sasl_getpath();
      int sasl_verifyfile( std::string const & file, sasl_verify_type_t type );
      int sasl_authorize( std::string const & user, std::string const & auth, std::string const & realm, struct propctx * propctx );
#ifdef INFOTROPE_ENABLE_XPERSIST
      Utils::magic_ptr<Session> const & session( std::string const & );
      void session( Utils::magic_ptr<Session> const & );
#endif
    public:
      static Master * instance;
      
    private:
      void log_real( int prio, std::string const & ) const;
      void send_control( char, std::string const & ) const;
      int listen( std::string const & );
      void close( std::string const & );
      void setup_datastore();
      void setup_sasl();
      void setup_addresses();
      
      bool load_dump( std::string const &, bool );
      bool load_ts( std::string const & );
      
    public:
      std::string const & datadir() const {
	return m_datadir;
      }
      
    private:
      int m_ctrlfd;
      typedef std::map< int, Infotrope::Utils::magic_ptr<Worker> > t_workers;
      t_workers m_workers;
      std::string m_datadir;
      std::deque<int> m_dead;
      Infotrope::Threading::Mutex m_lock;
      Infotrope::Threading::Mutex m_listen_lock;
      Infotrope::Utils::magic_ptr<Infotrope::Data::Datastore> m_ds;
      bool m_full_dump_rq;
#ifdef INFOTROPE_ENABLE_XPERSIST
      typedef std::map< std::string, Infotrope::Utils::magic_ptr<Infotrope::Server::Session> > t_sessions;
      t_sessions m_sessions;
      Infotrope::Threading::Mutex m_session_lock;
#endif
      pthread_t m_self;
    };
  }
}
#endif
