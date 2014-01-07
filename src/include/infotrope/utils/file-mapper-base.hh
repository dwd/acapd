#ifndef INFOTROPE_UTILS_FILE_MAPPER_BASE_HH
#define INFOTROPE_UTILS_FILE_MAPPER_BASE_HH

#include <string>
#include <map>
#include <infotrope/threading/lock.hh>

namespace Infotrope {
  
  namespace Utils {
    
    namespace MMap {
      
      typedef signed long int locality_type;
      
      template<typename T> class filemapper_ptr;
      
      class SimpleAlloc;
      
      class Manager {
      private:
	typedef std::map< locality_type, std::pair<unsigned long int,std::size_t> > t_loads;
	t_loads m_loads;
	std::string m_directory;
#ifdef _REENTRANT
	Infotrope::Threading::Mutex m_mutex;
#endif
      public:
	Manager( std::string const & base_directory );
	
	unsigned long int pointer( locality_type );
	filemapper_ptr<SimpleAlloc> simplealloc( std::size_t, locality_type );
      };
      
      class ScopeBlock {
      private:
	std::set< locality_type > m_thisloads;
      public:
	ScopeBlock();
	~ScopeBlock();
	
	void add( locality_type );
      };
      
      Manager * g_mmap_man;
      
    }
    
  }

}

#endif
