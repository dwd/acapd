#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/string-int.hh>

#include <iostream>

using namespace Infotrope::Datastore;

int main() {
  try {
    DataController dc( "bdb" );
    StringInt si( dc );
    {
      String foo( "entry" );
      std::cout << "Stored " << foo << std::endl;
    }
    {
      String foo( "subdataset" );
      std::cout << "Stored " << foo << std::endl;
    }
  } catch( std::exception & e ) {
    std::cout << e.what() << std::endl << std::flush;
  } catch( DbException & e ) {
    std::cout << e.what() << std::endl << std::flush;
  }
  return 0;
}
