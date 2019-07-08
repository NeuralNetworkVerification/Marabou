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

void printHelpMessage()
{
    std::cout << "input - Neural network file " << std::endl;
    std::cout << "property -  Property file " << std::endl;
    std::cout << "summary-file - Summary file " << std::endl;
    std::cout << "timeout - Global timeout " << std::endl;
    std::cout << "help - Prints the help message " << std::endl;
    std::cout << "version - Prints the version " << std::endl;
    std::cout << "pl-aux-eq - PL constraints generate auxiliary equations" <<std::endl;
    std::cout << "dnc - Use the divide-and-conquer solving mode " << std::endl;
    std::cout << "num-workers - (DNC) Number of workers " << std::endl;
    std::cout << "initial-divides - (DNC) Number of times to initially bisect the input region " << std::endl;
    std::cout << "initial-timeout - (DNC) The initial timeout " << std::endl;
    std::cout << "num-online-divides - (DNC) Number of times to further bisect a sub-region when a timeout occurs " << std::endl;
    std::cout << "timeout-factor - (DNC) The timeout factor " << std::endl;
    std::cout << "verbosity - Verbosity of engine::solve() " << std::endl; 
    std::cout << "\t 0: does not print anything (for DnC) " << std::endl;
    std::cout << "\t 1: print out statistics in the beginning and end " << std::endl;
    std::cout << "\t 2: print out statistics during solving. " << std::endl;

}

void printVersion()
{
    std::cout << "Marabou version 1.0.0 " << std::endl;
}

int main( int argc, char **argv )
{
    try
    {
        Options *options = Options::get();
        options->parseOptions( argc, argv );

        if (options->getBool( Options::HELP ) )
        {
            printHelpMessage();
            return 0;
        };

        if (options->getBool( Options::VERSION ) )
        {
            printVersion();
            return 0;
        };



        if ( options->getBool( Options::DNC_MODE ) )
            DnCMarabou().run();
        else
            Marabou().run();
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
