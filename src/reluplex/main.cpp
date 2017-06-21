/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include <cstdio>

#include "Error.h"
#include "InputQuery.h"

int main()
{
    try
    {
        printf( "Hello world!\n" );
    }
    catch ( const Error &e )
    {
        printf( "main.cpp: Error caught. Code: %u. Errno: %i. Class: %s. Message: %s\n",
                e.getCode(),
                e.getErrno(),
                e.getErrorClass(),
                e.getUserMessage() );
        fflush( 0 );
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
