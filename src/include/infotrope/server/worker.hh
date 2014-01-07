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

#ifndef INFOTROPE_SERVER_WORKER_HH
#define INFOTROPE_SERVER_WORKER_HH
#include <infotrope/threading/thread.hh>
#include <infotrope/threading/siglock.hh>
#include <infotrope/stream/tcpstream.hh>
#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/server/token.hh>
#include <infotrope/config.h>
#include <deque>
#include <stack>
#include <map>
#include <sasl/sasl.h>

namespace Infotrope {
  namespace Server {
    class Command;
    class Context;
#ifdef INFOTROPE_ENABLE_XPERSIST
    class Session;
#endif
    
    enum Scope {
      INIT,
      PREAUTH,
      AUTH
    };
    
    class Worker : public ::Infotrope::Threading::Thread {
    public:
      typedef enum {
	Terminated,
	Shutdown,
	NoMessage = -1
      } t_msg;
      Worker( int fd );
      virtual void * runtime();
      
      void log( int, std::string const & ) const;
      
      void banner();
      void sasl_setup();
      
      void waiting();
      void shutdown();
      void terminated();
      
      t_msg get_message( bool wait );
      void post_message( t_msg );
      
      void send( Infotrope::Utils::magic_ptr<Infotrope::Token::List> const &, bool=false );
      
      bool isWaiting() {
	return m_waiting;
      }
      
      Infotrope::Utils::magic_ptr<Infotrope::Token::List> const & current_line() {
	return m_toks;
      }
      
      void feed_me( Command * );
      
      bool starttls( std::string const & certfile );
      bool startdeflate();
    private:
      void parse_spool();
      void parse_line( bool=false );
      void new_command();
      
      void stuff_token( Infotrope::Utils::magic_ptr<Infotrope::Token::Token> const & );
      
      void stuff_qs();
      void stuff_literalmarker();
      void stuff_literal();
      void stuff_atom();
      void stuff_list();
      void pop_list();
      
      void clear_command_state();
      void reset_parser();
      void command_failure();
      
    public:
      void pop_scope() {
	m_scopes.pop_front();
      }
      void push_scope( Infotrope::Server::Scope s ) {
	m_scopes.push_front( s );
      }
      
      int fd() const {
	return m_fd;
      }
      
      std::string const & login() const;
      void login( std::string const & );
      
      // Context management.
      bool context_exists( std::string const & ) const;
      Utils::magic_ptr<Context> const & context( std::string const & ) const;
      void context_finished( Utils::magic_ptr<Context> const & );
      void context_register( Utils::magic_ptr<Context> const & );

#ifdef INFOTROPE_ENABLE_XPERSIST      
      // Session handling, for persistence.
      void session();
      void set_session();
      void set_session( std::string const & );
#endif
      
      sasl_conn_t * sasl_conn() const {
	return m_sasl_conn;
      }
      
    public:
      ~Worker();
      
    private:
      // Communications.
      int m_fd;
      std::deque<t_msg> m_msgs;
      Infotrope::Threading::SigMutex m_msglock;
      Infotrope::Threading::Mutex m_sendlock;
      Infotrope::Utils::magic_ptr<Infotrope::Stream::tcpstream> m_tcp;
      
      // Command parsing.
      Infotrope::Utils::magic_ptr<Infotrope::Token::List> m_toks;
      std::stack< Infotrope::Utils::magic_ptr<Infotrope::Token::List> > m_active;
      Infotrope::Utils::magic_ptr<Infotrope::Token::LiteralMarker> m_marker;
      std::string m_buf;
      unsigned long int m_spool_len;
      bool m_eol_seen;
      bool m_command_dead;
      bool m_quoted;
      bool m_escaped;
      bool m_skip_space;
      Infotrope::Utils::magic_ptr<Infotrope::Server::Command> m_command;
      bool m_dead;
      bool m_waiting;
      bool m_feeding;
      
      // Command dispatch.
      typedef std::map< std::string, Infotrope::Utils::magic_ptr<Infotrope::Server::Command> > t_commands;
      t_commands m_commands;
      
      // (And state)
      typedef std::deque<Infotrope::Server::Scope> t_scopes;
      t_scopes m_scopes;
      
      // Login name.
      std::string m_login;
      
      // SASL Connection
      sasl_conn_t * m_sasl_conn;
      
      // Contexts.
      Infotrope::Threading::Mutex m_cxt_lock;
      typedef std::map< std::string, Utils::magic_ptr<Context> > t_contexts;
      t_contexts m_contexts;

#ifdef INFOTROPE_ENABLE_XPERSIST      
      // Session.
      Infotrope::Utils::magic_ptr<Session> m_session;
      Infotrope::Threading::Mutex m_session_lock;
      void session_create();
#endif
    };
  }
}
#endif
