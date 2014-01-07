
#ifndef INFOTROPE_UTILS_SKIPLIST_HH
#define INFOTROPE_UTILS_SKIPLIST_HH

namespace Infotrope {
  
  namespace Utils {
    template< typename KeyPtrT, typename AllocT > class skiplist_node {
    public:
      typedef KeyPtrT pointer_type;
      typedef AllocT alloc_type;
      
      typedef typename alloc_type::rebind< skiplist_node< pointer_type, alloc_type > >::other node_alloc_type;
      typedef typename node_alloc_type::pointer_type node_pointer_type;
      
      typedef typename alloc_type::rebind< node_pointer_type >::other node_list_alloc_type;
      typedef typename node_list_alloc_type::pointer_type node_list_pointer_type;
    
    public:
      unsigned char node_height;
      pointer_type key;
      node_list_pointer_type fwd_ptrs;
    };
    
    template< typename KeyT, template CompT = std::less<KeyT>, template AllocT = std::allocator<KeyT> > class sl_set {
    public:
      typedef KeyT key_type;
      typedef CompT comparator_type;
      typedef AllocT allocator_type;
      typedef typename allocator_type::pointer_type pointer_type;
      typedef typename allocator_type::difference_type difference_type;
      typedef std::size_t size_type;
      typedef skiplist_node<key_type,allocator_type> node_type;
      
      typedef typename allocator_type::rebind<node_type>::other node_allocator_type;
      typedef typename node_allocator_type::pointer_type node_pointer_type;
      
      typedef typename allocator_type::rebind<node_pointer_type>::other node_pointer_allocator_type;
      
    private:
      size_type m_size;
      size_type m_bits;
      node_pointer_type m_start;
      comparator_type m_comp;
      
      node_pointer_allocator
    public:
      sl_set()
	: m_size(0), m_bits(1), m_start(0), m_comp() {
	}
      
    private:
      node_pointer_type int_search( key_type const & k ) {
	node_pointer_type x( m_start );
	
	for( size_type level( m_bits );
	     level>=0; --level ) {
	  while( m_comp( *(x->fwd_ptrs[level]->key), k ) ) {
	    x = x->fwd_ptrs[level];
	  }
	}
	if( m_comp( k, *(x->key) ) || m_comp( *(x->key), k ) ) {
	  return node_pointer_type( 0 );
	}
	return x;
      }
      
      node_pointer_type int_insert( key_type const & k ) {
	node_type update;
	update.fwd_ptrs = 
	
      
