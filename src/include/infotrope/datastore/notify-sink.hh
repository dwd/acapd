#ifndef INFOTROPE_DATASTORE_NOTIFY_SINK_HH
#define INFOTROPE_DATASTORE_NOTIFY_SINK_HH

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/utils/stringrep.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/threading/lock.hh>
#include <list>
#include <infotrope/datastore/notify-source.hh>

namespace Infotrope {
  
  namespace Notify {
    class Sink {
    public:
      Sink();
      virtual ~Sink();
      
      void notify_message( Utils::magic_ptr<Source> const &, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which );
      void process_all();
      virtual void handle_notify( Utils::magic_ptr<Source> const &, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which ) throw() = 0;
      virtual void handle_complete() throw();
    protected:
      void processing( bool b ) {
	Infotrope::Threading::Lock l__inst( m_lock );
	m_processing = b;
      }
      bool processing() const {
	return m_processing;
      }
    private:
      struct Message;
      std::list< Message * > m_messages;
      Infotrope::Threading::Mutex m_lock;
      bool m_processing;
    };
  }
}

#endif
