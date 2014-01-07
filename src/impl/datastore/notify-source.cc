#include <infotrope/datastore/notify-source.hh>
#include <infotrope/datastore/notify-sink.hh>
#include <infotrope/datastore/transaction.hh>
#include <infotrope/server/master.hh>
#include <algorithm>

using namespace Infotrope::Data;
using namespace Infotrope::Notify;
using namespace Infotrope::Server;
using namespace Infotrope;

Source::Source() : m_notify_sinks(), m_lock() {
}

Source::~Source() {
  // Need to notify sinks that we're vanishing.
}

void Source::register_sink( Utils::magic_ptr<Sink> const & ss ) {
  Infotrope::Threading::WriteLock l__inst( m_lock );
  Utils::weak_ptr<Sink> sw( ss );
  m_notify_sinks.insert( sw );
}

void Source::unregister_sink( Utils::weak_ptr<Sink> const & sw ) {
  Infotrope::Threading::WriteLock l__inst( m_lock );
  m_notify_sinks.erase( sw );
}

namespace {
  class RemoveDead {
  public:
    bool operator()( Utils::weak_ptr<Sink> const & ss ) {
      return !ss.harden();
    }
  };
}

void Source::notify( Utils::magic_ptr<Source> const & this_magic, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which ) {
  //Master::master()->log( 1, "Notifying sinks about '" + what.get() + "'" );
  Infotrope::Threading::ReadLock l__inst( m_lock );
  //Master::master()->log( 1, "Got lock." );
  {
    Infotrope::Threading::WriteLock l__inst( m_lock );
    if( m_notify_sinks.begin()==m_notify_sinks.end() ) {
      return;
    }
    RemoveDead rd;
    for( t_notify_sinks::iterator i( std::find_if( m_notify_sinks.begin(), m_notify_sinks.end(), rd ) );
	 i!=m_notify_sinks.end();
	 i=std::find_if( m_notify_sinks.begin(), m_notify_sinks.end(), rd ) ) {
      //Master::master()->log( 1, "Dead Sink." );
      m_notify_sinks.erase( i );
    }
  }
  //Master::master()->log( 1, "Notify loop starting." );
  for( t_notify_sinks::const_iterator i( m_notify_sinks.begin() ); i!=m_notify_sinks.end(); ++i ) {
    Utils::magic_ptr<Sink> ss( (*i).harden() );
    if( ss ) {
      //Master::master()->log( 1, "Notifying a sink." );
      ss->notify_message( this_magic, what, when, which );
      //Master::master()->log( 1, "Adding it to transaction." );
      Data::Transaction::add( ss );
      //ss->handle_notify( this_magic, what, when, which );
      //} else {
      //Master::master()->log( 1, "Still got dead Sink." );
    }
  }
  //Master::master()->log( 1, "Done notifying sinks." );
}
