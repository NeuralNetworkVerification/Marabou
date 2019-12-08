/*********************                                                        */
/*! \file LookAheadPreprocessor.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include <LookAheadPreprocessor.h>

#include <thread>

void LookAheadPreprocessor::run( Map<unsigned, unsigned> &bToPhase )
{
    bool progressMade = true;
    while ( progressMade )
    {

        std::cout << bToPhase.size() << std::endl;


    }
}

void LookAheadPreprocessor::createEngines()
{
    // Create the base engine
    _baseEngine = std::make_shared<Engine>( 0 );

    // Create engines for each thread
    for ( unsigned i = 0; i < _numWorkers; ++i )
    {
        auto engine = std::make_shared<Engine>( 0 );
        _engines.append( engine );
    }

    return true;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
