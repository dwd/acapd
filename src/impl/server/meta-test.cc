#include <unistd.h>
#include <string>

std::string msg( "L 0 Help\n" );
std::string msg2( "L 1 Help!\n" );

int main( int argc, char ** argv ) {
  sleep( 1 );
  write( 0, msg.c_str(), msg.length() );
  sleep( 1 );
  write( 0, msg2.c_str(), msg2.length() );
  sleep(1);
  return 0;
}
