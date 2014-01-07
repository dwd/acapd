#include <infotrope/server/log.hh>
#include <infotrope/server/master.hh>

void Infotrope::Server::Log( int prio, std::string const & s ) {
  Infotrope::Server::Master::master()->log( prio, s );
}
