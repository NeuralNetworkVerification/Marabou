/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "DnCMarabou.h"
#include "Error.h"
#include "Marabou.h"
#include "Options.h"

int main( int argc, char **argv )
{
    try
    {
        Options *options = Options::get();
        options->parseOptions( argc, argv );

        if ( options->getBool( Options::DNC_MODE ) )
            DnCMarabou().run();
        else
            Marabou( options->getInt( Options::VERBOSITY ) ).run();
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
