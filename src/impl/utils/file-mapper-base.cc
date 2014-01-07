#include <infotrope/utils/file-mapper.hh>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace Infotrope::Utils::MMap;

Manager::Manager( std::string const & directory )
  : m_directory( directory ) {
}

unsigned long int Manager::pointer( locality_type l ) {
  if( !l ) return 0;
#ifdef _REENTRANT
  Infotrope::Threading::Lock l__inst( m_mutex );
#endif
  t_loads::iterator i( m_loads.find( l ) );
  if( i==m_loads.end() ) {
    // Need to load this.
    std::ostringstream filename;
    filename << m_directory << "storage_" << l;
    int fd( open( filename.str().c_str(), O_RDWR ) );
    void * ptr(0);
    bool init( false );
    if( fd<0 ) {
      fd = open( filename.str().c_str(), O_RDWR|O_CREAT, 0666 );
      if( fd < 0 ) {
	throw std::bad_alloc();
      }
      init = true;
      ftruncate( fd, g_pagesize*1024 );
    }
    ptr = mmap( 0, g_pagesize*1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
    close( fd );
    if( init ) {
      new(ptr) Locality( l );
    }
    std::pair<t_loads::iterator,bool> loaded( m_loads.insert( std::make_pair( reinterpret_cast<unsigned long int>(ptr), std::make_pair( l, 0 ) ) ) );
    i = loaded.first;
  }
  // Update loaded information.
  ++(*i).second.second;
  return (*i).second.first;
}

filemapper_ptr<SimpleAlloc> Manager::simplealloc( std::size_t s, locality_type l ) {
  if( l<0 ) l=-l;
  filemapper_ptr<Locality> loc( l, 0 );
  return loc->simplealloc( s );
}
