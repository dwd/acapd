#include <infotrope/datastore/trans-logger.hh>
#include <infotrope/datastore/constants.hh>
#include <infotrope/datastore/dataset.hh>
#include <infotrope/datastore/entry.hh>

using namespace Infotrope::Data;
using namespace Infotrope::Constants;
using namespace Infotrope;

TransLogger::TransLogger() {
}

TransLogger::~TransLogger() {
}

void TransLogger::handle_notify( Utils::magic_ptr<Notify::Source> const & src, Utils::StringRep::entry_type const & what,
				 Modtime const & when, unsigned long int which ) throw() {
  Utils::magic_ptr<Dataset> dset( Utils::magic_cast<Dataset>( src ) );
  dump_spacer();
  m_xml << "<dataset name='" << dset->path() << "'>\n";
  m_spacer++;
  Utils::magic_ptr<Entry> ee( dset->requested_subcontext()->fetch2( what, true ) );
  if( ee ) {
    dump_entry( what.get(), *(ee) );
  } else {
    Entry e;
    e.store_default( c_attr_entry );
    dump_entry( what.get(), e );
  }
  m_spacer--;
  dump_spacer();
  m_xml << "</dataset>\n";
}
