
#ifndef INFOTROPE_UTILS_BASE64_HH
#define INFOTROPE_UTILS_BASE64_HH

#include <string>

namespace Infotrope {
  namespace Utils {
    namespace Base64 {
      std::string encode( std::string const & );
      std::string decode( std::string const & );
    }
  }
}

#endif
