#ifndef __T__Stdlib_h__
#define __T__Stdlib_h__

#include <cxxtest/Mock.h>
#include <stdlib.h>

CXXTEST_MOCK_GLOBAL( void *,
					 malloc,
					 ( size_t size ),
					 ( size ) );

CXXTEST_MOCK_VOID_GLOBAL( free,
						  ( void *ptr ),
						  ( ptr ) );

CXXTEST_MOCK_GLOBAL( void *,
                     realloc,
                     ( void *ptr, size_t size ),
                     ( ptr, size ) );

CXXTEST_MOCK_VOID_GLOBAL( srand,
                          ( unsigned seed ),
                          ( seed ) );

CXXTEST_MOCK_GLOBAL( int,
                     rand,
                     (),
                     () );

#endif // __T__Stdlib_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
