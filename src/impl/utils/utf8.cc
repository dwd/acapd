#include <infotrope/utils/utf8.hh>
#include <stdexcept>
#include <cctype>

bool Infotrope::Utils::Utf8::validate( std::string const & str, bool genexcept ) {
  try {
    int octets(0);
    for( std::string::size_type c( 0 ); c!=str.length(); ++c ) {
      unsigned short cc( static_cast<unsigned short>( str[c] ) );
      if( octets ) {
	if( (cc&0xC0)!=0x80 ) {
	  throw std::runtime_error( "Validation error: UTF-8 continuation character expected." );
	}
	--octets;
      } else {
	// Find bits.
	if( cc==0 ) {
	  throw std::runtime_error( "Validation error: NUL found." );
	}
	if( cc==0xFE || cc==0xFF ) {
	  throw std::runtime_error( "Validation error: Invalid UTF-8." );
	}
	if( cc&0x80 ) { // High bit set, MB sequence start.
	  if( c+1 > str.length() ) {
	    throw std::runtime_error( "Validation error: Truncated UTF-8." );
	  }
	  if( (cc&0xC0)==0x80 ) {
	    throw std::runtime_error( "Validation error: Unexpected UTF-8 continuation character." );
	  } else if( (cc&0xE0)==0xC0 ) {
	    octets=1;
	    if( (cc&0x1E)==0 ) {
	      throw std::runtime_error( "Validation error: Overlong UTF-8 sequence." );
	    }
	  } else if( cc&0xF0==0xE0 ) {
	    octets=2;
	    if( (cc&0x0F)==0 ) {
	      if( str[c+1]&0xE0==0 ) {
		throw std::runtime_error( "Validation error: Overlong UTF-8 sequence." );
	      }
	    }
	  } else if( (cc&0xF8)==0xF0 ) {
	    octets=3;
	    if( (cc&0x07)==0 ) {
	      if( (str[c+1]&0xF0)==0 ) {
		throw std::runtime_error( "Validation error: Overlong UTF-8 sequence." );
	      }
	    }
	  } else if( (cc&0xFC)==0xF8 ) {
	    octets=4;
	    if( (cc&0x03)==0 ) {
	      if( (str[c+1]&0xF8)==0 ) {
		throw std::runtime_error( "Validation error: Overlong UTF-8 sequence." );
	      }
	    }
	  } else if( (cc&0XFE)==0xFC ) {
	    octets=5;
	    if( (cc&0x01)==0 ) {
	      if( (str[c+1]&0xFC)==0 ) {
		throw std::runtime_error( "Validation error: Overlong UTF-8 sequence." );
	      }
	    }
	  } else {
	    throw std::runtime_error( "Validation error: Unknown MB start sequence." );
	  }
	} else {
	  // Character is an ascii one.
	  // Is it printable?
	  if( std::iscntrl( cc ) ) {
	    throw std::runtime_error( "Validation error: Unprintable ASCII character found." );
	  }
	}
      }
    }
  } catch( ... ) {
    if( genexcept ) {
      throw;
    }
    return false;
  }
  return true;
}
