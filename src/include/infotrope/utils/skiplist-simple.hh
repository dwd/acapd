namespace Infotrope {
  
  namespace Utils {
    
    template<typename K, typename T> class Skiplist {
    public:
      SkiplistNode<K, T> * m_header;
      std::size_t m_items;
      std::size_t m_highwater;
      
      Skiplist() : header(0), items(0) {
      }
      
      void insert( std::pair<K const,T> const & p ) {
	if( m_header==0 ) {
	  m_header = new SkiplistNode<K, T>( 1 );
	  m_items = 1;
	  SkiplistNode<K,T> * node( new SkiplistNode<K,T>( 1, p ) );
	  m_header[0] = node;
	} else {
	  SkiplistNode<K,T> * found( lower_bound( p.first ) );
	  if( found.payload.first<p.first ) { // LESS
	    std::size_t tmp( items );
	    std::size_t maxlevel(0);
	    while( tmp!=0 ) {
	      ++maxlevel;
	      items >>= 1;
	    }
	    std::size_t level(1);
	    while( level<maxlevel && rand()%1 ) ++level;
	    SkiplistNode<K,T> * node( new SkiplistNode<K,T>( level, p ) );
	    for( std::size_t l(0); l<found->size(); ++l ) {
	      
