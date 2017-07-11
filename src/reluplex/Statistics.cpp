/*********************                                                        */
/*! \file Statistics.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Statistics.h"
#include "Time.h"

Statistics::Statistics()
    : _numMainLoopIterations( 0 )
    , _numSimplexSteps( 0 )
    , _timeSimplexStepsMilli( 0 )
    , _numConstraintFixingSteps( 0 )
    , _currentStackDepth( 0 )
    , _numSplits( 0 )
    , _numPops( 0 )
{
}

void Statistics::print()
{
    printf( "\n%s Statistics update:\n", Time::now().ascii() );

    printf( "\tNumber of main loop iterators: %llu (%llu simplex steps, %llu constraint-fixing steps)\n"
            , _numMainLoopIterations
            , _numSimplexSteps
            , _numConstraintFixingSteps );

    printf( "\tTotal time performing simplex steps: %llu millisectonds. Average: %llu milliseconds\n"
            , _timeSimplexStepsMilli
            , printAverage( _timeSimplexStepsMilli, _numSimplexSteps ) );

    printf( "\tSMT core: total depth is %u. Number of splits: %u. Number of pops: %u\n"
            , _currentStackDepth
            , _numSplits
            , _numPops );
}

unsigned long long Statistics::printPercents( unsigned long long part, unsigned long long total ) const
{
    if ( total == 0 )
        return 0;

    return 100.0 * part / total;
}

unsigned long long Statistics::printAverage( unsigned long long part, unsigned long long total ) const
{
    if ( total == 0 )
        return 0;

    return (double)part / total;
}

void Statistics::incNumMainLoopIterations()
{
    ++_numMainLoopIterations;
}

void Statistics::incNumSimplexSteps()
{
    ++_numSimplexSteps;
}

void Statistics::addTimeSimplexSteps( unsigned long long time )
{
    _timeSimplexStepsMilli += time;
}

void Statistics::incNumConstraintFixingSteps()
{
    ++_numConstraintFixingSteps;
}

unsigned long long Statistics::getNumMainLoopIterations() const
{
    return _numMainLoopIterations;
}

void Statistics::setCurrentStackDepth( unsigned depth )
{
    _currentStackDepth = depth;
}

void Statistics::incNumSplits()
{
    ++_numSplits;
}

void Statistics::incNumPops()
{
    ++_numPops;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
