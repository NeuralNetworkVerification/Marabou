/*********************                                                        */
/*! \file stdlib.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

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
