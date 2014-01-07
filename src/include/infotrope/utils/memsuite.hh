#ifndef INFOTROPE_UTILS_MEMSUITE_H
#define INFOTROPE_UTILS_MEMSUITE_H

namespace Infotrope {
  namespace Memsuite {
    
#ifdef _REENTRANT
    void * mutex_alloc();
    int mutex_lock( void * );
    int mutex_release( void * );
    void mutex_free( void * );
#endif
    
    void * malloc( long unsigned int );
    void * malloc( unsigned int );
    void * realloc( void *, long unsigned int );
    void * realloc( void *, unsigned int );
    void * calloc( long unsigned int, long unsigned int );
    void free( void * );
  }
}

#endif
