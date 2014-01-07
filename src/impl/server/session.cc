#include <infotrope/server/session.hh>
#include <infotrope/server/master.hh>

using namespace Infotrope::Server;

Session::Session( std::string const & name, Worker & w )
  : m_name( name ), m_login( w.login() ), m_worker( Master::master()->get( w.fd() ) ) {
}

bool Session::context_exists( std::string const & c ) const {
  Threading::Lock l__inst( m_cxt_lock );
  return m_contexts.find( c ) != m_contexts.end();
}

Infotrope::Utils::magic_ptr<Context> const & Session::context( std::string const & c ) {
  Threading::Lock l__inst( m_cxt_lock );
  t_contexts::const_iterator i( m_contexts.find( c ) );
  if( i==m_contexts.end() ) {
    throw std::runtime_error( "No such context " + c );
  }
  return (*i).second;
}

void Session::context_finished( Utils::magic_ptr<Context> const & c ) {
  Threading::Lock l__inst( m_cxt_lock );
  m_contexts.erase( c->name() );
}

void Session::context_register( Utils::magic_ptr<Context> const & c ) {
  Threading::Lock l__inst( m_cxt_lock );
  m_contexts[c->name()] = c;
}

void Session::bind( Worker & wx ) {
  if( m_worker.harden() ) {
    throw std::runtime_error( "Session already bound." );
  }
  Utils::magic_ptr<Worker> w( Master::master()->get( wx.fd() ) );
  m_worker = w;
  for( t_contexts::const_iterator i( m_contexts.begin() ); i!=m_contexts.end(); ++i ) {
    (*i).second->bind( wx );
  }
}

bool Session::bound() const {
  return m_worker.harden();
}

void Session::unbind() {
  for( t_contexts::const_iterator i( m_contexts.begin() ); i!=m_contexts.end(); ++i ) {
    (*i).second->unbind();
  }
}
