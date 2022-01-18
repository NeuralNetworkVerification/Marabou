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
    , _timedOut( false )
{
    _unsignedAttributes[NUM_PL_CONSTRAINTS] = 0;
    _unsignedAttributes[NUM_ACTIVE_PL_CONSTRAINTS] = 0;
    _unsignedAttributes[NUM_PL_VALID_SPLITS] = 0;
    _unsignedAttributes[NUM_PL_SMT_ORIGINATED_SPLITS] = 0;
    _unsignedAttributes[NUM_PRECISION_RESTORATIONS] = 0;
    _unsignedAttributes[CURRENT_DECISION_LEVEL] = 0;
    _unsignedAttributes[MAX_DECISION_LEVEL] = 0;
    _unsignedAttributes[NUM_SPLITS] = 0;
    _unsignedAttributes[NUM_POPS] = 0;
    _unsignedAttributes[NUM_VISITED_TREE_STATES] = 1;
    _unsignedAttributes[CURRENT_TABLEAU_M] = 0;
    _unsignedAttributes[CURRENT_TABLEAU_N] = 0;
    _unsignedAttributes[PP_NUM_ELIMINATED_VARS] = 0;
    _unsignedAttributes[PP_NUM_TIGHTENING_ITERATIONS] = 0;
    _unsignedAttributes[PP_NUM_CONSTRAINTS_REMOVED] = 0;
    _unsignedAttributes[PP_NUM_EQUATIONS_REMOVED] = 0;
    _unsignedAttributes[TOTAL_NUMBER_OF_VALID_CASE_SPLITS] = 0;

    _longAttributes[NUM_MAIN_LOOP_ITERATIONS] = 0;
    _longAttributes[NUM_SIMPLEX_STEPS] = 0;
    _longAttributes[TIME_SIMPLEX_STEPS_MICRO] = 0;
    _longAttributes[TIME_MAIN_LOOP_MICRO] = 0;
    _longAttributes[TIME_CONSTRAINT_FIXING_STEPS_MICRO] = 0;
    _longAttributes[NUM_CONSTRAINT_FIXING_STEPS] = 0;
    _longAttributes[NUM_TABLEAU_PIVOTS] = 0;
    _longAttributes[NUM_TABLEAU_DEGENERATE_PIVOTS] = 0;
    _longAttributes[NUM_TABLEAU_DEGENERATE_PIVOTS_BY_REQUEST] = 0;
    _longAttributes[TIME_PIVOTS_MICRO] = 0;
    _longAttributes[NUM_SIMPLEX_PIVOT_SELECTIONS_IGNORED_FOR_STABILITY] = 0;
    _longAttributes[NUM_SIMPLEX_UNSTABLE_PIVOTS] = 0;
    _longAttributes[NUM_ADDED_ROWS] = 0;
    _longAttributes[NUM_MERGED_COLUMNS] = 0;
    _longAttributes[NUM_TABLEAU_BOUND_HOPPING] = 0;
    _longAttributes[NUM_TIGHTENED_BOUNDS] = 0;
    _longAttributes[NUM_TIGHTENINGS_FROM_SYMBOLIC_BOUND_TIGHTENING] = 0;
    _longAttributes[NUM_ROWS_EXAMINED_BY_ROW_TIGHTENER] = 0;
    _longAttributes[NUM_TIGHTENINGS_FROM_ROWS] = 0;
    _longAttributes[NUM_BOUND_TIGHTENINGS_ON_EXPLICIT_BASIS] = 0;
    _longAttributes[NUM_TIGHTENINGS_FROM_EXPLICIT_BASIS] = 0;
    _longAttributes[NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS] = 0;
    _longAttributes[NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS] = 0;
    _longAttributes[NUM_BOUNDS_PROPOSED_BY_PL_CONSTRAINTS] = 0;
    _longAttributes[NUM_BOUND_TIGHTENINGS_ON_CONSTRAINT_MATRIX] = 0;
    _longAttributes[NUM_TIGHTENINGS_FROM_CONSTRAINT_MATRIX] = 0;
    _longAttributes[NUM_BASIS_REFACTORIZATIONS] = 0;
    _longAttributes[PSE_NUM_ITERATIONS] = 0;
    _longAttributes[PSE_NUM_RESET_REFERENCE_SPACE] = 0;
    _longAttributes[TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO] = 0;
    _longAttributes[TOTAL_TIME_PERFORMING_SYMBOLIC_BOUND_TIGHTENING] = 0;
    _longAttributes[TOTAL_TIME_HANDLING_STATISTICS_MICRO] = 0;
    _longAttributes[TOTAL_TIME_EXPLICIT_BASIS_BOUND_TIGHTENING_MICRO] = 0;
    _longAttributes[TOTAL_TIME_DEGRADATION_CHECKING] = 0;
    _longAttributes[TOTAL_TIME_PRECISION_RESTORATION] = 0;
    _longAttributes[TOTAL_TIME_CONSTRAINT_MATRIX_BOUND_TIGHTENING_MICRO] = 0;
    _longAttributes[TOTAL_TIME_APPLYING_STORED_TIGHTENINGS_MICRO] = 0;
    _longAttributes[TOTAL_TIME_SMT_CORE_MICRO] = 0;

    _doubleAttributes[CURRENT_DEGRADATION] = 0.0;
    _doubleAttributes[MAX_DEGRADATION] = 0.0;
}

void Statistics::stampStartingTime()
{
    _startTime = TimeUtils::sampleMicro();
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
            , _currentDecisionLevel
            , _numVisitedTreeStates
            , _numSplits
            , _numPops );
    printf( "\tMax stack depth: %u\n"
            , _maxDecisionLevel );

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

unsigned long long Statistics::getTotalTime() const
{
    return TimeUtils::timePassed( _startTime, TimeUtils::sampleMicro() );
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
