#include <infotrope/datastore/notify-sink.hh>
#include <infotrope/server/master.hh>

using namespace Infotrope::Notify;
using namespace Infotrope::Data;
using namespace Infotrope::Server;
using namespace Infotrope;

Sink::Sink() : m_processing( true ) {
}

struct Sink::Message {
  Message( Utils::magic_ptr<Source> const & as, Utils::StringRep::entry_type const & awhat, Data::Modtime const & awhen, unsigned long int awhich )
    : s( as ), what( awhat ), when( awhen ), which( awhich ) {
  }
  Utils::magic_ptr<Source> s;
  Utils::StringRep::entry_type what;
  Data::Modtime when;
  unsigned long int which;
};

Sink::~Sink() {
  for( std::list<Message *>::iterator i( m_messages.begin() ); i!=m_messages.end(); ++i ) {
    delete (*i);
  }
  m_messages.clear();
}

void Sink::notify_message( Utils::magic_ptr<Source> const & s, Utils::StringRep::entry_type const & what, Data::Modtime const & when, unsigned long int which ) {
  Infotrope::Threading::Lock l__inst( m_lock );
  m_messages.push_back( new Sink::Message( s, what, when, which ) );
}

void Sink::process_all() {
  //Master::master()->log( 1, "Processing all changes." );
  Infotrope::Threading::Lock l__inst( m_lock );
  //Master::master()->log( 1, "Obtaining lock." );
  if( !m_processing ) {
    return;
  }
  {
    for( std::list< Message * >::const_iterator i( m_messages.begin() ); i!=m_messages.end(); ++i ) {
      //Master::master()->log( 1, "Handle_notify..." );
      handle_notify( (*i)->s, (*i)->what, (*i)->when, (*i)->which );
      //Master::master()->log( 1, "Done." );
      delete (*i);
      //Master::master()->log( 1, "Deleted." );
    }
    if( !m_messages.empty() ) {
      handle_complete();
    }
    m_messages.clear();
  }
  //Master::master()->log( 1, "Complete." );
}

// Default implementation does nothing.
void Sink::handle_complete() throw() {
}
