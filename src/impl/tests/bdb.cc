#include <iostream>
#include <string>
#include <db_cxx.h>
#include <stdexcept>

#include <infotrope/datastore/datastore.hh>
#include <infotrope/datastore/transaction.hh>

int stringrep_callback( Db * db, const Dbt * key, const Dbt * data, Dbt * res ) {
  res->set_data( data->get_data() );
  res->set_size( data->get_size() );
  return 0;
}

using namespace Infotrope::Datastore;

class Test {
 public:
  Test( DataController & dc ) : m_strings( &dc.env(), 0 ), m_strings_decode( &dc.env(), 0 ) {
    Transaction trans;
    //dc.env().txn_begin( 0, &txn, DB_TXN_SYNC );
    
    m_strings.open( trans.txn(), "strings", "strmaster", DB_BTREE, DB_THREAD|DB_CREATE, 0600 );
    
    m_strings_decode.open( trans.txn(), "strings", "strid", DB_HASH, DB_THREAD|DB_CREATE, 0600 );
    
    m_strings.associate( trans.txn(), &m_strings_decode, stringrep_callback, DB_CREATE );
    
    //int ret( txn->commit( DB_TXN_SYNC ) );
    //std::cout << "Create-Commit returned " << db_strerror( ret ) << std::endl;
    trans.commit();
  }
  
  std::pair<bool,unsigned long> get_key( std::string const & what_foo ) {
    std::string add_str( what_foo.data(), what_foo.length() ); // Force data copy.
    Transaction trans;
    unsigned long res( 0 );
    Dbt key( const_cast<char *>(add_str.data()), add_str.length() );
    unsigned long f;
    Dbt data( &f, sizeof(f) );
    data.set_ulen( sizeof(f) );
    data.set_flags( DB_DBT_USERMEM );
    int ret( m_strings.get( trans.txn(), &key, &data, 0 ) );
    if( ret==0 ) {
      res = f;
      trans.commit();
      return std::make_pair( true, res );
    }
    trans.commit();
    return std::make_pair<bool,unsigned long>( false, 0 );
  }
  
  void put_key( std::string const & foo, unsigned long f ) {
    std::string add_str( foo.data(), foo.length() );
    Dbt key( const_cast<char *>(add_str.data()), add_str.length() );
    unsigned long stringrep_count( f );
    Dbt data( &stringrep_count, sizeof(stringrep_count) );
    Transaction trans;
    int ret( m_strings.put( trans.txn(), &key, &data, 0 ) );
    if( ret!=0 ) {
      std::cerr << "Aborting." << std::endl;
      //txn->abort();
      throw std::runtime_error( "Foo1");
    } else {
      std::cout << "Commit.\n" << std::flush;
      //int ret( txn->commit( DB_TXN_SYNC ) );
      //std::cout << "Data-Commit returned " << db_strerror( ret ) << std::endl << std::flush;
      trans.commit();
    }
  }
  
  unsigned long id( std::string const & s ) {
    Transaction trans;
    std::pair<bool,unsigned long> r( get_key( s ) );
    if( r.first ) {
      trans.commit();
      return r.second;
    } else {
      std::pair<bool,unsigned long> hr( get_key( "_highest" ) );
      unsigned long highest(0);
      if( hr.first ) {
	highest = hr.second;
      }
      put_key( "_highest", highest+1 );
      put_key( s, highest );
      trans.commit();
      return highest;
    }
  }
  
  ~Test() {
    m_strings_decode.close( 0 );
    m_strings.close( 0 );
  }
  
 private:
  Db m_strings;
  Db m_strings_decode;
};

int main( int argc, char ** argv ) {
  try {
    //DbEnv dbe( 0 );
    //dbe.open( "bdb", DB_INIT_MPOOL|DB_INIT_TXN|DB_INIT_LOCK|DB_INIT_LOG|DB_CREATE|DB_RECOVER|DB_THREAD, 0600 );
    DataController dc( "bdb" );
    std::string add_str( "entry" );
    if( argc>1 ) {
      add_str = argv[1];
    }
    Test t( dc );
    {
      unsigned long f( t.id( add_str ) );
      std::cout << add_str << " now " << f << std::endl << std::flush;
    }
  } catch( std::exception & e ) {
    std::cerr << "Standard Exception caught: " << e.what() << std::endl;
  } catch( DbException & e ) {
    std::cerr << "DB Exception caught: " << e.what() << std::endl;
  } catch(...) {
    std::cerr << "Unknown exception caught. Bummer." << std::endl;
  }
}
