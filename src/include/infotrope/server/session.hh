#ifndef INFOTROPE_SERVER_SESSION_HH
#define INFOTROPE_SERVER_SESSION_HH

#include <string>
#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/server/worker.hh>
#include <infotrope/server/context.hh>

namespace Infotrope {
  
  namespace Server {
    
    class Session {
    public:
      Session( std::string const & name, Worker & w );
      ~Session() {
      }
      
      std::string const & name() const {
	return m_name;
      }
      std::string const & login() const {
	return m_login;
      }
      
      bool context_exists( std::string const & ) const;
      Utils::magic_ptr<Context> const & context( std::string const & name );
      void context_finished( Utils::magic_ptr<Context> const & );
      void context_register( Utils::magic_ptr<Context> const & );
      
      void bind( Worker & w );
      void unbind();
      bool bound() const;
    private:
      std::string const m_name;
      std::string const m_login;
      Infotrope::Utils::weak_ptr<Worker> m_worker;
      Infotrope::Threading::Mutex m_cxt_lock;
      typedef std::map< std::string, Utils::magic_ptr<Context> > t_contexts;
      t_contexts m_contexts;
    };
    
  }
  
}

#endif
