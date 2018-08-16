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

#include "Error.h"
#include "Marabou.h"

int main( int argc, char **argv )
{
    try
    {
        Marabou().run( argc, argv );
    }
    catch ( const Error &e )
    {
        printf( "Caught a %s error. Code: %u, Errno: %i, Message: %s.\n",
                e.getErrorClass(),
                e.getCode(),
                e.getErrno(),
                e.getUserMessage() );

        return 1;
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
