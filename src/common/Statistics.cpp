/*********************                                                        */
/*! \file Statistics.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

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
    , _numPrecisionRestorations( 0 )
    , _numSimplexSteps( 0 )
    , _timeSimplexStepsMicro( 0 )
    , _timeMainLoopMicro( 0 )
    , _timeConstraintFixingStepsMicro( 0 )
    , _numConstraintFixingSteps( 0 )
    , _currentStackDepth( 0 )
    , _maxStackDepth( 0 )
    , _numSplits( 0 )
    , _numPops( 0 )
    , _numVisitedTreeStates( 1 )
    , _numTableauPivots( 0 )
    , _numTableauDegeneratePivots( 0 )
    , _numTableauDegeneratePivotsByRequest( 0 )
    , _timePivotsMicro( 0 )
    , _numSimplexPivotSelectionsIgnoredForStability( 0 )
    , _numSimplexUnstablePivots( 0 )
    , _numAddedRows( 0 )
    , _numMergedColumns( 0 )
    , _currentTableauM( 0 )
    , _currentTableauN( 0 )
    , _numTableauBoundHopping( 0 )
    , _numTightenedBounds( 0 )
    , _numTighteningsFromSymbolicBoundTightening( 0 )
    , _numRowsExaminedByRowTightener( 0 )
    , _numTighteningsFromRows( 0 )
    , _numBoundTighteningsOnExplicitBasis( 0 )
    , _numTighteningsFromExplicitBasis( 0 )
    , _numBoundNotificationsToPlConstraints( 0 )
    , _numBoundsProposedByPlConstraints( 0 )
    , _numBoundTighteningsOnConstraintMatrix( 0 )
    , _numTighteningsFromConstraintMatrix( 0 )
    , _numBasisRefactorizations( 0 )
    , _pseNumIterations( 0 )
    , _pseNumResetReferenceSpace( 0 )
    , _ppNumEliminatedVars( 0 )
    , _ppNumTighteningIterations( 0 )
    , _ppNumConstraintsRemoved( 0 )
    , _ppNumEquationsRemoved( 0 )
    , _totalTimePerformingValidCaseSplitsMicro( 0 )
    , _totalTimePerformingSymbolicBoundTightening( 0 )
    , _totalTimeHandlingStatisticsMicro( 0 )
    , _totalNumberOfValidCaseSplits( 0 )
    , _totalTimeExplicitBasisBoundTighteningMicro( 0 )
    , _totalTimeDegradationChecking( 0 )
    , _totalTimePrecisionRestoration( 0 )
    , _totalTimeConstraintMatrixBoundTighteningMicro( 0 )
    , _totalTimeApplyingStoredTighteningsMicro( 0 )
    , _totalTimeSmtCoreMicro( 0 )
    , _timedOut( false )
{
}

void Statistics::print()
{
    printf( "\n%s Statistics update:\n", TimeUtils::now().ascii() );

    struct timespec now = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( _startTime, now );

    unsigned seconds = totalElapsed / 1000000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;

    printf( "\t--- Time Statistics ---\n" );
    printf( "\tTotal time elapsed: %llu milli (%02u:%02u:%02u)\n",
            totalElapsed / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    seconds = _timeMainLoopMicro / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tMain loop: %llu milli (%02u:%02u:%02u)\n",
            _timeMainLoopMicro / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    seconds = _preprocessingTimeMicro / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tPreprocessing time: %llu milli (%02u:%02u:%02u)\n",
            _preprocessingTimeMicro / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    unsigned long long totalUnknown = totalElapsed - _timeMainLoopMicro - _preprocessingTimeMicro;

    seconds = totalUnknown / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tUnknown: %llu milli (%02u:%02u:%02u)\n",
            totalUnknown / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    printf( "\tBreakdown for main loop:\n" );
    printf( "\t\t[%.2lf%%] Simplex steps: %llu milli\n"
            , printPercents( _timeSimplexStepsMicro, _timeMainLoopMicro )
            , _timeSimplexStepsMicro / 1000
            );
    printf( "\t\t[%.2lf%%] Explicit-basis bound tightening: %llu milli\n"
            , printPercents( _totalTimeExplicitBasisBoundTighteningMicro, _timeMainLoopMicro )
            , _totalTimeExplicitBasisBoundTighteningMicro / 1000
            );
    printf( "\t\t[%.2lf%%] Constraint-matrix bound tightening: %llu milli\n"
            , printPercents( _totalTimeConstraintMatrixBoundTighteningMicro, _timeMainLoopMicro )
            , _totalTimeConstraintMatrixBoundTighteningMicro / 1000
            );
    printf( "\t\t[%.2lf%%] Degradation checking: %llu milli\n"
            , printPercents( _totalTimeDegradationChecking, _timeMainLoopMicro )
            , _totalTimeDegradationChecking / 1000
            );
    printf( "\t\t[%.2lf%%] Precision restoration: %llu milli\n"
            , printPercents( _totalTimePrecisionRestoration, _timeMainLoopMicro )
            , _totalTimePrecisionRestoration / 1000
            );
    printf( "\t\t[%.2lf%%] Statistics handling: %llu milli\n"
            , printPercents( _totalTimeHandlingStatisticsMicro, _timeMainLoopMicro )
            , _totalTimeHandlingStatisticsMicro / 1000
            );
    printf( "\t\t[%.2lf%%] Constraint-fixing steps: %llu milli\n"
            , printPercents( _timeConstraintFixingStepsMicro, _timeMainLoopMicro )
            , _timeConstraintFixingStepsMicro / 1000
            );
    printf( "\t\t[%.2lf%%] Valid case splits: %llu milli. Average per split: %.2lf milli\n"
            , printPercents( _totalTimePerformingValidCaseSplitsMicro, _timeMainLoopMicro )
            , _totalTimePerformingValidCaseSplitsMicro / 1000
            , printAverage( _totalTimePerformingValidCaseSplitsMicro / 1000,
                            _totalNumberOfValidCaseSplits )
            );
    printf( "\t\t[%.2lf%%] Applying stored bound-tightening: %llu milli\n"
            , printPercents( _totalTimeApplyingStoredTighteningsMicro, _timeMainLoopMicro )
            , _totalTimeApplyingStoredTighteningsMicro / 1000
            );

    printf( "\t\t[%.2lf%%] SMT core: %llu milli\n"
            , printPercents( _totalTimeSmtCoreMicro, _timeMainLoopMicro )
            , _totalTimeSmtCoreMicro / 1000
            );

    printf( "\t\t[%.2lf%%] Symbolic Bound Tightening: %llu milli\n"
            , printPercents( _totalTimePerformingSymbolicBoundTightening, _timeMainLoopMicro )
            , _totalTimePerformingSymbolicBoundTightening / 1000
            );

    unsigned long long total =
        _timeSimplexStepsMicro +
        _timeConstraintFixingStepsMicro +
        _totalTimePerformingValidCaseSplitsMicro +
        _totalTimeHandlingStatisticsMicro +
        _totalTimeExplicitBasisBoundTighteningMicro +
        _totalTimeDegradationChecking +
        _totalTimePrecisionRestoration +
        _totalTimeConstraintMatrixBoundTighteningMicro +
        _totalTimeApplyingStoredTighteningsMicro +
        _totalTimeSmtCoreMicro +
        _totalTimePerformingSymbolicBoundTightening;

    printf( "\t\t[%.2lf%%] Unaccounted for: %llu milli\n"
            , printPercents( _timeMainLoopMicro - total, _timeMainLoopMicro )
            , _timeMainLoopMicro > total ? ( _timeMainLoopMicro - total ) / 1000 : 0
            );

    printf( "\t--- Preprocessor Statistics ---\n" );
    printf( "\tNumber of preprocessor bound-tightening loop iterations: %u\n",
            _ppNumTighteningIterations );
    printf( "\tNumber of eliminated variables: %u\n",
            _ppNumEliminatedVars );
    printf( "\tNumber of constraints removed due to variable elimination: %u\n",
            _ppNumConstraintsRemoved );
    printf( "\tNumber of equations removed due to variable elimination: %u\n",
            _ppNumEquationsRemoved );

    printf( "\t--- Engine Statistics ---\n" );
    printf( "\tNumber of main loop iterations: %llu\n"
            "\t\t%llu iterations were simplex steps. Total time: %llu milli. Average: %.2lf milli.\n"
            "\t\t%llu iterations were constraint-fixing steps. "
            "Total time: %llu milli. Average: %.2lf milli\n"
            , _numMainLoopIterations
            , _numSimplexSteps
            , _timeSimplexStepsMicro / 1000
            , printAverage( _timeSimplexStepsMicro / 1000, _numSimplexSteps )
            , _numConstraintFixingSteps
            , _timeConstraintFixingStepsMicro / 1000
            , printAverage( _timeConstraintFixingStepsMicro / 1000, _numConstraintFixingSteps )
            );
    printf( "\tNumber of active piecewise-linear constraints: %u / %u\n"
            "\t\tConstraints disabled by valid splits: %u. "
            "By SMT-originated splits: %u\n"
            , _numActivePlConstraints
            , _numPlConstraints
            , _numPlValidSplits
            , _numPlSmtOriginatedSplits
            );
    printf( "\tLast reported degradation: %.10lf. Max degradation so far: %.10lf. "
            "Restorations so far: %u\n"
            , _currentDegradation
            , _maxDegradation
            , _numPrecisionRestorations
            );
    printf( "\tNumber of simplex pivots we attempted to skip because of instability: %llu.\n"
            "\tUnstable pivots performed anyway: %llu\n"
            , _numSimplexPivotSelectionsIgnoredForStability
            , _numSimplexUnstablePivots );

    printf( "\t--- Tableau Statistics ---\n" );
    printf( "\tTotal number of pivots performed: %llu\n", _numTableauPivots );
    printf( "\t\tReal pivots: %llu. Degenerate: %llu (%.2lf%%)\n"
            , _numTableauPivots - _numTableauDegeneratePivots
            , _numTableauDegeneratePivots
            , printPercents( _numTableauDegeneratePivots, _numTableauPivots ) );

    printf( "\t\tDegenerate pivots by request (e.g., to fix a PL constraint): %llu (%.2lf%%)\n"
            , _numTableauDegeneratePivotsByRequest
            , printPercents( _numTableauDegeneratePivotsByRequest, _numTableauDegeneratePivots ) );

    printf( "\t\tAverage time per pivot: %.2lf milli\n",
            printAverage( _timePivotsMicro / 1000, _numTableauPivots ) );

    printf( "\tTotal number of fake pivots performed: %llu\n", _numTableauBoundHopping );
    printf( "\tTotal number of rows added: %llu. Number of merged columns: %llu\n"
            , _numAddedRows
            , _numMergedColumns );
    printf( "\tCurrent tableau dimensions: M = %u, N = %u\n"
            , _currentTableauM
            , _currentTableauN );

    printf( "\t--- SMT Core Statistics ---\n" );
    printf( "\tTotal depth is %u. Total visited states: %u. Number of splits: %u. Number of pops: %u\n"
            , _currentStackDepth
            , _numVisitedTreeStates
            , _numSplits
            , _numPops );
    printf( "\tMax stack depth: %u\n"
            , _maxStackDepth );

    printf( "\t--- Bound Tightening Statistics ---\n" );
    printf( "\tNumber of tightened bounds: %llu.\n", _numTightenedBounds );
    printf( "\t\tNumber of rows examined by row tightener: %llu. Consequent tightenings: %llu\n"
            , _numRowsExaminedByRowTightener
            , _numTighteningsFromRows );

    printf( "\t\tNumber of explicit basis matrices examined by row tightener: %llu. Consequent tightenings: %llu\n"
            , _numBoundTighteningsOnExplicitBasis
            , _numTighteningsFromExplicitBasis );

    printf( "\t\tNumber of bound tightening rounds on the entire constraint matrix: %llu. "
            "Consequent tightenings: %llu\n"
            , _numBoundTighteningsOnConstraintMatrix
            , _numTighteningsFromConstraintMatrix );

    printf( "\t\tNumber of bound notifications sent to PL constraints: %llu. Tightenings proposed: %llu\n"
            , _numBoundNotificationsToPlConstraints
            , _numBoundsProposedByPlConstraints );

    printf( "\t--- Basis Factorization statistics ---\n" );
    printf( "\tNumber of basis refactorizations: %llu\n",
            _numBasisRefactorizations );

    printf( "\t--- Projected Steepest Edge Statistics ---\n" );
    printf( "\tNumber of iterations: %llu.\n", _pseNumIterations );
    printf( "\tNumber of resets to reference space: %llu. Avg. iterations per reset: %u\n"
            , _pseNumResetReferenceSpace
            , _pseNumResetReferenceSpace > 0 ?
            (unsigned)((double)_pseNumIterations / _pseNumResetReferenceSpace) : 0 );

    printf( "\t--- SBT ---\n" );
    printf( "\tNumber of tightened bounds: %llu\n", _numTighteningsFromSymbolicBoundTightening );
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

void Statistics::incNumPrecisionRestorations()
{
    ++_numPrecisionRestorations;
}

void Statistics::addTimeSimplexSteps( unsigned long long time )
{
    _timeSimplexStepsMicro += time;
}

void Statistics::addTimeMainLoop( unsigned long long time )
{
    _timeMainLoopMicro += time;
}

void Statistics::addTimeConstraintFixingSteps( unsigned long long time )
{
    _timeConstraintFixingStepsMicro += time;
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

    if ( _currentStackDepth > _maxStackDepth )
        _maxStackDepth = _currentStackDepth;
}

unsigned Statistics::getMaxStackDepth() const
{
    return _maxStackDepth;
}

void Statistics::incNumSplits()
{
    ++_numSplits;
}

void Statistics::incNumPops()
{
    ++_numPops;
}

unsigned Statistics::getNumPops() const
{
    return _numPops;
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

void Statistics::addTimePivots( unsigned long long time )
{
    _timePivotsMicro += time;
}

unsigned Statistics::getAveragePivotTimeInMicro() const
{
    if ( _numTableauPivots == 0 )
        return 0;

    return _timePivotsMicro / _numTableauPivots;
}

void Statistics::incNumSimplexPivotSelectionsIgnoredForStability()
{
    ++_numSimplexPivotSelectionsIgnoredForStability;
}

void Statistics::incNumSimplexUnstablePivots()
{
    ++_numSimplexUnstablePivots;
}

void Statistics::incNumAddedRows()
{
    ++_numAddedRows;
}

void Statistics::incNumMergedColumns()
{
    ++_numMergedColumns;
}

void Statistics::setCurrentTableauDimension( unsigned m, unsigned n )
{
    _currentTableauM = m;
    _currentTableauN = n;
}

void Statistics::incNumTightenedBounds()
{
    ++_numTightenedBounds;
}

void Statistics::incNumRowsExaminedByRowTightener()
{
    ++_numRowsExaminedByRowTightener;
}

void Statistics::incNumTighteningsFromRows( unsigned increment )
{
    _numTighteningsFromRows += increment;
}

void Statistics::incNumBoundTighteningsOnExplicitBasis()
{
    ++_numBoundTighteningsOnExplicitBasis;
}

void Statistics::incNumTighteningsFromExplicitBasis( unsigned increment )
{
    _numTighteningsFromExplicitBasis += increment;
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

void Statistics::incNumTighteningsFromConstraintMatrix( unsigned increment )
{
    _numTighteningsFromConstraintMatrix += increment;
}

void Statistics::incNumBasisRefactorizations()
{
    ++_numBasisRefactorizations;
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

void Statistics::setPreprocessingTime( unsigned long long micro )
{
    _preprocessingTimeMicro = micro;
}

void Statistics::stampStartingTime()
{
    _startTime = TimeUtils::sampleMicro();
}

void Statistics::ppSetNumEliminatedVars( unsigned eliminatedVars )
{
    _ppNumEliminatedVars = eliminatedVars;
}

void Statistics::ppIncNumTighteningIterations()
{
    ++_ppNumTighteningIterations;
}

void Statistics::ppIncNumConstraintsRemoved()
{
    ++_ppNumConstraintsRemoved;
}

void Statistics::ppIncNumEquationsRemoved()
{
    ++_ppNumEquationsRemoved;
}

void Statistics::addTimeForValidCaseSplit( unsigned long long time )
{
    _totalTimePerformingValidCaseSplitsMicro += time;
    ++_totalNumberOfValidCaseSplits;
}

void Statistics::addTimeForSymbolicBoundTightening( unsigned long long time )
{
    _totalTimePerformingSymbolicBoundTightening += time;
}

void Statistics::addTimeForStatistics( unsigned long long time )
{
    _totalTimeHandlingStatisticsMicro += time;
}

void Statistics::addTimeForExplicitBasisBoundTightening( unsigned long long time )
{
    _totalTimeExplicitBasisBoundTighteningMicro += time;
}

void Statistics::addTimeForDegradationChecking( unsigned long long time )
{
    _totalTimeDegradationChecking += time;
}

void Statistics::addTimeForPrecisionRestoration( unsigned long long time )
{
    _totalTimePrecisionRestoration += time;
}

void Statistics::addTimeForConstraintMatrixBoundTightening( unsigned long long time )
{
    _totalTimeConstraintMatrixBoundTighteningMicro += time;
}

void Statistics::addTimeForApplyingStoredTightenings( unsigned long long time )
{
    _totalTimeApplyingStoredTighteningsMicro += time;
}

void Statistics::addTimeSmtCore( unsigned long long time )
{
    _totalTimeSmtCoreMicro += time;
}

void Statistics::incNumVisitedTreeStates()
{
    ++_numVisitedTreeStates;
}

unsigned Statistics::getNumVisitedTreeStates() const
{
    return _numVisitedTreeStates;
}

unsigned Statistics::getNumSplits() const
{
    return _numSplits;
}

unsigned long long Statistics::getNumTableauPivots() const
{
    return _numTableauPivots;
}

double Statistics::getMaxDegradation() const
{
    return _maxDegradation;
}

unsigned Statistics::getNumPrecisionRestorations() const
{
    return _numPrecisionRestorations;
}

unsigned long long Statistics::getTimeSimplexStepsMicro() const
{
    return _timeSimplexStepsMicro;
}

unsigned long long Statistics::getNumConstraintFixingSteps() const
{
    return _numConstraintFixingSteps;
}

unsigned long long Statistics::getNumSimplexPivotSelectionsIgnoredForStability() const
{
    return _numSimplexPivotSelectionsIgnoredForStability;
}

unsigned long long Statistics::getNumSimplexUnstablePivots() const
{
    return _numSimplexUnstablePivots;
}

unsigned long long Statistics::getTotalTime() const
{
    unsigned long long total =
        _timeSimplexStepsMicro +
        _timeConstraintFixingStepsMicro +
        _totalTimePerformingValidCaseSplitsMicro +
        _totalTimeHandlingStatisticsMicro +
        _totalTimeExplicitBasisBoundTighteningMicro +
        _totalTimeDegradationChecking +
        _totalTimePrecisionRestoration +
        _totalTimeConstraintMatrixBoundTighteningMicro +
        _totalTimeApplyingStoredTighteningsMicro +
        _totalTimeSmtCoreMicro +
        _totalTimePerformingSymbolicBoundTightening;

    // Total is in micro seconds, and we need to return milliseconds
    return total / 1000;
}

void Statistics::timeout()
{
    _timedOut = true;
}

bool Statistics::hasTimedOut() const
{
    return _timedOut;
}

void Statistics::printStartingIteration( unsigned long long iteration, String message )
{
    if ( _numMainLoopIterations >= iteration )
        printf( "DBG_PRINT: %s\n", message.ascii() );
}

void Statistics::incNumTighteningsFromSymbolicBoundTightening( unsigned increment )
{
    _numTighteningsFromSymbolicBoundTightening += increment;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
