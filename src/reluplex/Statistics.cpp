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
#include "TimeUtils.h"

Statistics::Statistics()
    : _numMainLoopIterations( 0 )
    , _numPlConstraints( 0 )
    , _numActivePlConstraints( 0 )
    , _numSimplexSteps( 0 )
    , _timeSimplexStepsMilli( 0 )
    , _numConstraintFixingSteps( 0 )
    , _currentStackDepth( 0 )
    , _numSplits( 0 )
    , _numPops( 0 )
    , _numTableauPivots( 0 )
    , _numTableauDegeneratePivots( 0 )
    , _numTableauDegeneratePivotsByRequest( 0 )
    , _numTableauBoundHopping( 0 )
    , _numTightenedBounds( 0 )
    , _numRowsExaminedByTightener( 0 )
{
}

void Statistics::print()
{
    printf( "\n%s Statistics update:\n", TimeUtils::now().ascii() );

    printf( "\t--- Engine Statistics ---\n" );
    printf( "\tNumber of main loop iterations: %llu\n"
            "\t\t%llu iterations were simplex steps. Total time: %llu milli. Average: %.2lf milli.\n"
            "\t\t%llu iterations were constraint-fixing steps.\n"
            , _numMainLoopIterations
            , _numSimplexSteps
            , _timeSimplexStepsMilli
            , printAverage( _timeSimplexStepsMilli, _numSimplexSteps )
            , _numConstraintFixingSteps );
    printf( "\tNumber of active piecewise-linear constraints: %u / %u\n"
            , _numActivePlConstraints
            , _numPlConstraints );

    printf( "\t--- Tableau Statistics ---\n" );
    printf( "\tTotal number of pivots performed: %llu\n", _numTableauPivots );
    printf( "\t\tReal pivots: %llu. Degenerate: %llu (%.2lf%%)\n"
            , _numTableauPivots - _numTableauDegeneratePivots
            , _numTableauDegeneratePivots
            , printPercents( _numTableauDegeneratePivots, _numTableauPivots ) );

    printf( "\t\tDenegerate pivots by request (e.g., to fix a PL constraint): %llu (%.2lf%%)\n"
            , _numTableauDegeneratePivotsByRequest
            , printPercents( _numTableauDegeneratePivotsByRequest, _numTableauDegeneratePivots ) );

    printf( "\t--- SMT Core Statistics ---\n" );
    printf( "\tTotal depth is %u. Number of splits: %u. Number of pops: %u\n"
            , _currentStackDepth
            , _numSplits
            , _numPops );

    printf( "\t--- Bound Tightener Statistics ---\n" );
    printf( "\tNumber of tightened bounds: %llu. Number of rows examined: %llu\n"
            , _numTightenedBounds
            , _numRowsExaminedByTightener );
}

double Statistics::printPercents( unsigned long long part, unsigned long long total ) const
{
    if ( total == 0 )
        return 0;

    return 100.0 * part / total;
}

double Statistics::printAverage( unsigned long long part, unsigned long long total ) const
{
    if ( total == 0 )
        return 0;

    return (double)part / total;
}

void Statistics::incNumMainLoopIterations()
{
    ++_numMainLoopIterations;
}

void Statistics::setNumPlConstraints( unsigned numberOfConstraints )
{
    _numPlConstraints = numberOfConstraints;
}

void Statistics::setNumActivePlConstraints( unsigned numberOfConstraints )
{
    _numActivePlConstraints = numberOfConstraints;
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

void Statistics::incNumTableauPivots()
{
    ++_numTableauPivots;
}

void Statistics::incNumTableauBoundHopping()
{
    ++_numTableauBoundHopping;
}

void Statistics::incNumTableauDegeneratePivots()
{
    ++_numTableauDegeneratePivots;
}

void Statistics::incNumTableauDegeneratePivotsByRequest()
{
    ++_numTableauDegeneratePivotsByRequest;
}

void Statistics::incNumTightenedBounds()
{
    ++_numTightenedBounds;
}

void Statistics::incNumRowsExaminedByTightener()
{
    ++_numRowsExaminedByTightener;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
