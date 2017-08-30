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

#include "FloatUtils.h"
#include "Statistics.h"
#include "TimeUtils.h"

Statistics::Statistics()
    : _numMainLoopIterations( 0 )
    , _numPlConstraints( 0 )
    , _numActivePlConstraints( 0 )
    , _numPlValidSplits( 0 )
    , _numPlSmtOriginatedSplits( 0 )
    , _currentDegradation( 0.0 )
    , _maxDegradation( 0.0 )
    , _numSimplexSteps( 0 )
    , _timeSimplexStepsMilli( 0 )
    , _numConstraintFixingSteps( 0 )
    , _currentStackDepth( 0 )
    , _numSplits( 0 )
    , _numPops( 0 )
    , _numTableauPivots( 0 )
    , _numTableauDegeneratePivots( 0 )
    , _numTableauDegeneratePivotsByRequest( 0 )
    , _numSimplexPivotSelectionsIgnoredForStability( 0 )
    , _numSimplexUnstablePivots( 0 )
    , _numTableauBoundHopping( 0 )
    , _numTightenedBounds( 0 )
    , _numRowsExaminedByRowTightener( 0 )
    , _numBoundsProposedByRowTightener( 0 )
    , _numBoundNotificationsToPlConstraints( 0 )
    , _numBoundsProposedByPlConstraints( 0 )
    , _numBoundTighteningsOnConstraintMatrix( 0 )
    , _pseNumIterations( 0 )
    , _pseNumResetReferenceSpace( 0 )
{
}

void Statistics::print()
{
    printf( "\n%s Statistics update:\n", TimeUtils::now().ascii() );

    timeval now = TimeUtils::sampleMicro();

    unsigned long long mainLoopMilli = TimeUtils::timePassed( _startTime, now );
    unsigned long long totalMilli = mainLoopMilli + _preprocessingTimeMilli;

    unsigned seconds = totalMilli / 1000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;
    printf( "\tTotal time elapsed: %llu milli (%02u:%02u:%02u)\n",
            totalMilli, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    seconds = mainLoopMilli / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tMain loop: %llu milli (%02u:%02u:%02u)\n",
            mainLoopMilli, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    seconds = _preprocessingTimeMilli / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tPreprocessing time: %llu milli (%02u:%02u:%02u)\n",
            _preprocessingTimeMilli, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

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
            "\t\tConstraints disabled by valid splits: %u. By smt-originated splits: %u\n"
            , _numActivePlConstraints
            , _numPlConstraints
            , _numPlValidSplits
            , _numPlSmtOriginatedSplits
            );
    printf( "\tLast reported degradation: %.5lf. Max degradation so far: %.5lf\n"
            , _currentDegradation
            , _maxDegradation
            );
    printf( "\tNumber of simplex pivots we attempted to skip beacuse of instability: %llu. "
            "Unstable pivots performed anyway: %llu\n"
            , _numSimplexPivotSelectionsIgnoredForStability
            , _numSimplexUnstablePivots );

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

    printf( "\t--- Bound Tighetning Statistics ---\n" );
    printf( "\tNumber of tightened bounds: %llu.\n", _numTightenedBounds );
    printf( "\t\tNumber of rows examined by row tightener: %llu. Tightenings proposed: %llu\n"
            , _numRowsExaminedByRowTightener
            , _numBoundsProposedByRowTightener );
    printf( "\t\tNumber of bound notifications sent to PL constraints: %llu. Tightenings proposed: %llu\n"
            , _numBoundNotificationsToPlConstraints
            , _numBoundsProposedByPlConstraints );
    printf( "\t\tNumber of bound tightening rounds on the entire constraint matrix: %llu\n"
            , _numBoundTighteningsOnConstraintMatrix );

    printf( "\t--- Projected Steepest Edge Statistics ---\n" );
    printf( "\tNumber of iterations: %llu.\n", _pseNumIterations );
    printf( "\tNumber of resets to reference space: %llu. Avg. iterations per reset: %u\n"
            , _pseNumResetReferenceSpace
            , _pseNumResetReferenceSpace > 0 ?
            (unsigned)((double)_pseNumIterations / _pseNumResetReferenceSpace) : 0 );
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

void Statistics::setNumPlValidSplits( unsigned numberOfSplits )
{
    _numPlValidSplits = numberOfSplits;
}

void Statistics::setNumPlSMTSplits( unsigned numberOfSplits )
{
    _numPlSmtOriginatedSplits = numberOfSplits;
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

void Statistics::incNumSimplexPivotSelectionsIgnoredForStability()
{
    ++_numSimplexPivotSelectionsIgnoredForStability;
}

void Statistics::incNumSimplexUnstablePivots()
{
    ++_numSimplexUnstablePivots;
}

void Statistics::incNumTightenedBounds()
{
    ++_numTightenedBounds;
}

void Statistics::incNumRowsExaminedByRowTightener()
{
    ++_numRowsExaminedByRowTightener;
}

void Statistics::incNumBoundsProposedByRowTightener()
{
    ++_numBoundsProposedByRowTightener;
}

void Statistics::incNumBoundNotificationsPlConstraints()
{
    ++_numBoundNotificationsToPlConstraints;
}

void Statistics::incNumBoundsProposedByPlConstraints()
{
    ++_numBoundsProposedByPlConstraints;
}

void Statistics::incNumBoundTighteningOnConstraintMatrix()
{
    ++_numBoundTighteningsOnConstraintMatrix;
}

void Statistics::pseIncNumIterations()
{
    ++_pseNumIterations;
}

void Statistics::pseIncNumResetReferenceSpace()
{
    ++_pseNumResetReferenceSpace;
}

void Statistics::setCurrentDegradation( double degradation )
{
    _currentDegradation = degradation;
    if ( FloatUtils::gt( _currentDegradation, _maxDegradation ) )
        _maxDegradation = _currentDegradation;
}

void Statistics::setPreprocessingTime( unsigned long long milli )
{
    _preprocessingTimeMilli = milli;
}

void Statistics::stampStartingTime()
{
    _startTime = TimeUtils::sampleMicro();
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
