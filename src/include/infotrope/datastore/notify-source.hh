#ifndef INFOTROPE_DATASTORE_NOTIFY_SOURCE_HH
#define INFOTROPE_DATASTORE_NOTIFY_SOURCE_HH

#include <infotrope/utils/magic-ptr.hh>
#include <infotrope/utils/stringrep.hh>
#include <infotrope/datastore/modtime.hh>
#include <infotrope/threading/rw-lock.hh>
#include <set>

namespace Infotrope {
  
  namespace Notify {
    class Sink;
    
    class Source {
    public:
      Source();
      virtual ~Source();
      
      void register_sink( Utils::magic_ptr<Sink> const & );
      void unregister_sink( Utils::weak_ptr<Sink> const & );
      void notify( Utils::magic_ptr<Source> const &, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which=0 );
      
    private:
      typedef std::set< Utils::weak_ptr<Sink> > t_notify_sinks;
      t_notify_sinks m_notify_sinks;
      Infotrope::Threading::RWMutex m_lock;
    };
  }
}

#endif
