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

static std::string getCompiler() {
    std::stringstream ss;
#ifdef __GNUC__
    ss << "GCC";
#else /* __GNUC__ */
    ss << "unknown compiler";
#endif /* __GNUC__ */
#ifdef __VERSION__
    ss << " version " << __VERSION__;
#else /* __VERSION__ */
    ss << ", unknown version";
#endif /* __VERSION__ */
    return ss.str();
}

static std::string getCompiledDateTime() {
    return __DATE__ " " __TIME__;
}

void printVersion()
{
    std::cout <<
        "Marabou version " << MARABOU_VERSION <<
        " [" << GIT_BRANCH << " " << GIT_COMMIT_HASH << "]"
	      << "\ncompiled with " << getCompiler()
	      << "\non " << getCompiledDateTime()
	      << std::endl;
}

void printHelpMessage()
{
    printVersion();
    std::cout << std::endl << "Usage: ./build/Marabou --input=NeuralNetFile --property=PropertyFile" << std::endl << std::endl;
    std::cout << "\t--input - Neural network file " << std::endl;
    std::cout << "\t--property -  Property file " << std::endl;
    std::cout << "\t--input-query - InputQuery file " << std::endl;
    std::cout << "\t--summary-file - Summary file " << std::endl;
    std::cout << "\t--timeout - Global timeout " << std::endl;
    std::cout << "\t--help - Prints the help message " << std::endl;
    std::cout << "\t--version - Prints the version " << std::endl;
    std::cout << "\t--pl-aux-eq - PL constraints generate auxiliary equations" <<std::endl;
    std::cout << "\t--dnc - Use the divide-and-conquer solving mode " << std::endl;
    std::cout << "\t--num-workers - (DNC) Number of workers " << std::endl;
    std::cout << "\t--initial-divides - (DNC) Number of initial bisections over input range" << std::endl;
    std::cout << "\t--initial-timeout - (DNC) The initial timeout " << std::endl;
    std::cout << "\t--num-online-divides - (DNC) Number of further bisections after a timeout" << std::endl;
    std::cout << "\t--timeout-factor - (DNC) The timeout factor " << std::endl;
    std::cout << "\t--verbosity - Verbosity of engine::solve() " << std::endl;
    std::cout << "\t\t 0: does not print anything (recommended for DNC mode)," << std::endl;
    std::cout << "\t\t 1: print out statistics in the beginning and the end," << std::endl;
    std::cout << "\t\t 2: print out statistics during solving. " << std::endl;
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

        if ( options->getBool( Options::DNC_MODE ) || options->getBool( Options::SPLIT_ONLY ) )
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
