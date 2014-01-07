#include <infotrope/utils/base64.hh>
#include <sasl/saslutil.h>
#include <stdexcept>

namespace {
  unsigned long int const plain_len( 1024 ); // Must be divisible by 4.
  unsigned long int const base64_len( 4 + ((plain_len*4)/3) ); // Every three octets encodes into four characters.
  // My maths is terrible, add 4 to be safe.
}

std::string Infotrope::Utils::Base64::encode( std::string const & s ) {
  char outbuf[base64_len];
  std::string ret;
  for( std::string::size_type p(0); p<s.length(); p+=plain_len ) {
    std::string::size_type count( plain_len );
    if( (p+plain_len) > s.length() ) {
      count = s.length()-p;
    }
    unsigned outlen(0);
    int r( sasl_encode64( s.data()+p, count, outbuf, base64_len, &outlen ) );
    if( r==SASL_BADPROT ) {
      throw std::runtime_error( "Bad base64 in " + std::string( s.data()+p, count ) );
    } else if( r==SASL_BUFOVER ) {
      throw std::runtime_error( "My apologies, I cannot do maths." );
    } else if( r!=SASL_OK ) {
      throw std::runtime_error( "Whoops, unknown error while encoding base64." );
    }
    ret += std::string( outbuf, outlen );
  }
  return ret;
}

std::string Infotrope::Utils::Base64::decode( std::string const & s ) {
  char outbuf[plain_len];
  std::string ret;
  for( std::string::size_type p(0); p<s.length(); p+=base64_len ) {
    std::string::size_type count( base64_len );
    if( (p+base64_len) > s.length() ) {
      count = s.length()-p;
    }
    unsigned outlen(0);
    int r( sasl_decode64( s.data()+p, count, outbuf, plain_len, &outlen ) );
    if( r==SASL_BADPROT ) {
      throw std::runtime_error( "Bad base64 in " + std::string( s.data()+p, count ) );
    } else if( r==SASL_BUFOVER ) {
      throw std::runtime_error( "My apologies, I cannot do maths." );
    } else if( r!=SASL_OK ) {
      throw std::runtime_error( "Whoops, unknown error while encoding base64." );
    }
    ret += std::string( outbuf, outlen );
  }
  return ret;
}
