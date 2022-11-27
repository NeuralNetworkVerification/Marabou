/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze Wu
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

#ifdef ENABLE_OPENBLAS
#include "cblas.h"
#endif

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
    Options::get()->printHelpMessage();
}

int main( int argc, char **argv )
{
    try
    {
        Options *options = Options::get();
        options->parseOptions( argc, argv );

        if ( options->getBool( Options::HELP ) )
        {
            printHelpMessage();
            return 0;
        };

        if ( options->getBool( Options::VERSION ) )
        {
            printVersion();
            return 0;
        };

        if ( Options::get()->getBool( Options::PRODUCE_PROOFS ) )
        {
            GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = false;
            options->setBool( Options::NO_PARALLEL_DEEPSOI, true );
            printf( "Proof production is not yet supported with DEEPSOI search, turning search off.\n" );
        }

        if ( Options::get()->getBool( Options::PRODUCE_PROOFS ) && ( options->getBool( Options::DNC_MODE ) ) )
        {
            options->setBool( Options::DNC_MODE, false );
            printf( "Proof production is not yet supported with snc mode, turning snc off.\n" );
        }

        if ( Options::get()->getBool( Options::PRODUCE_PROOFS ) && ( options->getBool( Options::SOLVE_WITH_MILP ) ) )
        {
            options->setBool( Options::SOLVE_WITH_MILP, false );
            printf( "Proof production is not yet supported with MILP solvers, turning SOLVE_WITH_MILP off.\n" );
        }

        if ( options->getBool( Options::DNC_MODE ) ||
             ( !options->getBool( Options::NO_PARALLEL_DEEPSOI ) &&
               !options->getBool( Options::SOLVE_WITH_MILP ) &&
               options->getInt( Options::NUM_WORKERS ) > 1 ) )
            DnCMarabou().run();
        else
	{
#ifdef ENABLE_OPENBLAS
	    openblas_set_num_threads( options->getInt( Options::NUM_BLAS_THREADS ) );
#endif
            Marabou().run();
	}
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
