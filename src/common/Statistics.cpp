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
    : _timedOut( false )
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

    unsigned long long timeMainLoopMicro = getLongAttribute
        ( Statistics::TIME_MAIN_LOOP_MICRO );
    seconds = timeMainLoopMicro / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tMain loop: %llu milli (%02u:%02u:%02u)\n",
            timeMainLoopMicro / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    unsigned long long preprocessingTimeMicro = getLongAttribute
        ( Statistics::PREPROCESSING_TIME_MICRO );
    seconds = preprocessingTimeMicro / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tPreprocessing time: %llu milli (%02u:%02u:%02u)\n",
            preprocessingTimeMicro / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    unsigned long long totalUnknown = totalElapsed - timeMainLoopMicro - preprocessingTimeMicro;

    seconds = totalUnknown / 1000000;
    minutes = seconds / 60;
    hours = minutes / 60;
    printf( "\t\tUnknown: %llu milli (%02u:%02u:%02u)\n",
            totalUnknown / 1000, hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ) );

    printf( "\tBreakdown for main loop:\n" );
    unsigned long long timeSimplexStepsMicro =
        getLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO );
    printf( "\t\t[%.2lf%%] Simplex steps: %llu milli\n"
            , printPercents( timeSimplexStepsMicro, timeMainLoopMicro )
            , timeSimplexStepsMicro / 1000
            );
    unsigned long long totalTimeExplicitBasisBoundTighteningMicro =
        getLongAttribute(TOTAL_TIME_EXPLICIT_BASIS_BOUND_TIGHTENING_MICRO );
    printf( "\t\t[%.2lf%%] Explicit-basis bound tightening: %llu milli\n"
            , printPercents( totalTimeExplicitBasisBoundTighteningMicro, timeMainLoopMicro )
            , totalTimeExplicitBasisBoundTighteningMicro / 1000
            );
    unsigned long long totalTimeConstraintMatrixBoundTighteningMicro =
        getLongAttribute(TOTAL_TIME_CONSTRAINT_MATRIX_BOUND_TIGHTENING_MICRO );
    printf( "\t\t[%.2lf%%] Constraint-matrix bound tightening: %llu milli\n"
            , printPercents( totalTimeConstraintMatrixBoundTighteningMicro, timeMainLoopMicro )
            , totalTimeConstraintMatrixBoundTighteningMicro / 1000
            );
    unsigned long long totalTimeDegradationChecking =
        getLongAttribute( Statistics::TOTAL_TIME_DEGRADATION_CHECKING );
    printf( "\t\t[%.2lf%%] Degradation checking: %llu milli\n"
            , printPercents( totalTimeDegradationChecking, timeMainLoopMicro )
            , totalTimeDegradationChecking / 1000
            );
    unsigned long long totalTimePrecisionRestoration =
        getLongAttribute( Statistics::TOTAL_TIME_PRECISION_RESTORATION );
    printf( "\t\t[%.2lf%%] Precision restoration: %llu milli\n"
            , printPercents( totalTimePrecisionRestoration, timeMainLoopMicro )
            , totalTimePrecisionRestoration / 1000
            );
    unsigned long long totalTimeHandlingStatisticsMicro =
        getLongAttribute( Statistics::TOTAL_TIME_HANDLING_STATISTICS_MICRO );
    printf( "\t\t[%.2lf%%] Statistics handling: %llu milli\n"
            , printPercents( totalTimeHandlingStatisticsMicro, timeMainLoopMicro )
            , totalTimeHandlingStatisticsMicro / 1000
            );
    unsigned long long timeConstraintFixingStepsMicro =
        getLongAttribute( Statistics::TIME_CONSTRAINT_FIXING_STEPS_MICRO );
    printf( "\t\t[%.2lf%%] Constraint-fixing steps: %llu milli\n"
            , printPercents( timeConstraintFixingStepsMicro, timeMainLoopMicro )
            , timeConstraintFixingStepsMicro / 1000
            );
    unsigned long long totalTimePerformingValidCaseSplitsMicro =
        getLongAttribute( Statistics::TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO );
    unsigned totalNumberOfValidCaseSplits =
        getUnsignedAttribute( Statistics::TOTAL_NUMBER_OF_VALID_CASE_SPLITS );
    printf( "\t\t[%.2lf%%] Valid case splits: %llu milli. Average per split: %.2lf milli\n"
            , printPercents( totalTimePerformingValidCaseSplitsMicro, timeMainLoopMicro )
            , totalTimePerformingValidCaseSplitsMicro / 1000
            , printAverage( totalTimePerformingValidCaseSplitsMicro / 1000,
                            totalNumberOfValidCaseSplits )
            );
    unsigned long long totalTimeApplyingStoredTighteningsMicro =
        getLongAttribute( Statistics::TOTAL_TIME_APPLYING_STORED_TIGHTENINGS_MICRO );
    printf( "\t\t[%.2lf%%] Applying stored bound-tightening: %llu milli\n"
            , printPercents( totalTimeApplyingStoredTighteningsMicro, timeMainLoopMicro )
            , totalTimeApplyingStoredTighteningsMicro / 1000
            );
    unsigned long long totalTimeSmtCoreMicro =
        getLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO );
    printf( "\t\t[%.2lf%%] SMT core: %llu milli\n"
            , printPercents( totalTimeSmtCoreMicro, timeMainLoopMicro )
            , totalTimeSmtCoreMicro / 1000
            );
    unsigned long long totalTimePerformingSymbolicBoundTightening =
        getLongAttribute( Statistics::TOTAL_TIME_PERFORMING_SYMBOLIC_BOUND_TIGHTENING );
    printf( "\t\t[%.2lf%%] Symbolic Bound Tightening: %llu milli\n"
            , printPercents( totalTimePerformingSymbolicBoundTightening, timeMainLoopMicro )
            , totalTimePerformingSymbolicBoundTightening / 1000
            );

    unsigned long long total =
        timeSimplexStepsMicro +
        timeConstraintFixingStepsMicro +
        totalTimePerformingValidCaseSplitsMicro +
        totalTimeHandlingStatisticsMicro +
        totalTimeExplicitBasisBoundTighteningMicro +
        totalTimeDegradationChecking +
        totalTimePrecisionRestoration +
        totalTimeConstraintMatrixBoundTighteningMicro +
        totalTimeApplyingStoredTighteningsMicro +
        totalTimeSmtCoreMicro +
        totalTimePerformingSymbolicBoundTightening;

    printf( "\t\t[%.2lf%%] Unaccounted for: %llu milli\n"
            , printPercents( timeMainLoopMicro - total, timeMainLoopMicro )
            , timeMainLoopMicro > total ? ( timeMainLoopMicro - total ) / 1000 : 0
            );

    printf( "\t--- Preprocessor Statistics ---\n" );
    printf( "\tNumber of preprocessor bound-tightening loop iterations: %u\n",
            getUnsignedAttribute( Statistics::PP_NUM_TIGHTENING_ITERATIONS ) );
    printf( "\tNumber of eliminated variables: %u\n",
            getUnsignedAttribute( Statistics::PP_NUM_ELIMINATED_VARS ) );
    printf( "\tNumber of constraints removed due to variable elimination: %u\n",
            getUnsignedAttribute( Statistics::PP_NUM_CONSTRAINTS_REMOVED ) );
    printf( "\tNumber of equations removed due to variable elimination: %u\n",
            getUnsignedAttribute( Statistics::PP_NUM_EQUATIONS_REMOVED ) );

    unsigned long long numSimplexSteps =
        getLongAttribute( Statistics::NUM_SIMPLEX_STEPS );
    unsigned long long numConstraintFixingSteps =
        getLongAttribute( Statistics::NUM_CONSTRAINT_FIXING_STEPS );

    printf( "\t--- Engine Statistics ---\n" );
    printf( "\tNumber of main loop iterations: %llu\n"
            "\t\t%llu iterations were simplex steps. Total time: %llu milli. Average: %.2lf milli.\n"
            "\t\t%llu iterations were constraint-fixing steps. "
            "Total time: %llu milli. Average: %.2lf milli\n"
            , getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS )
            , numSimplexSteps
            , timeSimplexStepsMicro / 1000
            , printAverage( timeSimplexStepsMicro / 1000, numSimplexSteps )
            , numConstraintFixingSteps
            , timeConstraintFixingStepsMicro / 1000
            , printAverage( timeConstraintFixingStepsMicro / 1000,
                            numConstraintFixingSteps )
            );
    printf( "\tNumber of active piecewise-linear constraints: %u / %u\n"
            "\t\tConstraints disabled by valid splits: %u. "
            "By SMT-originated splits: %u\n"
            , getUnsignedAttribute( Statistics::NUM_ACTIVE_PL_CONSTRAINTS )
            , getUnsignedAttribute( Statistics::NUM_PL_CONSTRAINTS )
            , getUnsignedAttribute( Statistics::NUM_PL_VALID_SPLITS )
            , getUnsignedAttribute( Statistics::NUM_PL_SMT_ORIGINATED_SPLITS )
            );
    printf( "\tLast reported degradation: %.10lf. Max degradation so far: %.10lf. "
            "Restorations so far: %u\n"
            , getDoubleAttribute( Statistics::CURRENT_DEGRADATION )
            , getDoubleAttribute( Statistics::MAX_DEGRADATION )
            , getUnsignedAttribute( Statistics::NUM_PRECISION_RESTORATIONS )
            );
    printf( "\tNumber of simplex pivots we attempted to skip because of instability: %llu.\n"
            "\tUnstable pivots performed anyway: %llu\n"
            , getLongAttribute( Statistics::NUM_SIMPLEX_PIVOT_SELECTIONS_IGNORED_FOR_STABILITY )
            , getLongAttribute( Statistics::NUM_SIMPLEX_UNSTABLE_PIVOTS ) );

    unsigned long long numTableauPivots = getLongAttribute(NUM_TABLEAU_PIVOTS );
    unsigned long long numTableauDegeneratePivots =
        getLongAttribute(NUM_TABLEAU_DEGENERATE_PIVOTS );
    printf( "\t--- Tableau Statistics ---\n" );
    printf( "\tTotal number of pivots performed: %llu\n", numTableauPivots );
    printf( "\t\tReal pivots: %llu. Degenerate: %llu (%.2lf%%)\n"
            , numTableauPivots - numTableauDegeneratePivots
            , numTableauDegeneratePivots
            , printPercents( numTableauDegeneratePivots, numTableauPivots ) );

    unsigned long long numTableauDegeneratePivotsByRequest =
        getLongAttribute(NUM_TABLEAU_DEGENERATE_PIVOTS_BY_REQUEST );
    printf( "\t\tDegenerate pivots by request (e.g., to fix a PL constraint): %llu (%.2lf%%)\n"
            , numTableauDegeneratePivotsByRequest
            , printPercents( numTableauDegeneratePivotsByRequest, numTableauDegeneratePivots ) );

    printf( "\t\tAverage time per pivot: %.2lf milli\n",
            printAverage( getLongAttribute( Statistics::TIME_PIVOTS_MICRO ) / 1000,
                          numTableauPivots ) );

    printf( "\tTotal number of fake pivots performed: %llu\n",
            getLongAttribute( Statistics::NUM_TABLEAU_BOUND_HOPPING ) );
    printf( "\tTotal number of rows added: %llu. Number of merged columns: %llu\n"
            , getLongAttribute( Statistics::NUM_ADDED_ROWS )
            , getLongAttribute( Statistics::NUM_MERGED_COLUMNS ) );
    printf( "\tCurrent tableau dimensions: M = %u, N = %u\n"
            , getUnsignedAttribute( Statistics::CURRENT_TABLEAU_M )
            , getUnsignedAttribute( Statistics::CURRENT_TABLEAU_N ) );

    printf( "\t--- SMT Core Statistics ---\n" );
    printf( "\tTotal depth is %u. Total visited states: %u. Number of splits: %u. Number of pops: %u\n"
            , getUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL )
            , getUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES )
            , getUnsignedAttribute( Statistics::NUM_SPLITS )
            , getUnsignedAttribute( Statistics::NUM_POPS ) );
    printf( "\tMax stack depth: %u\n"
            , getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) );

    printf( "\t--- Bound Tightening Statistics ---\n" );
    printf( "\tNumber of tightened bounds: %llu.\n",
            getLongAttribute( Statistics::NUM_TIGHTENED_BOUNDS ) );
    printf( "\t\tNumber of rows examined by row tightener: %llu. Consequent tightenings: %llu\n"
            , getLongAttribute( Statistics::NUM_ROWS_EXAMINED_BY_ROW_TIGHTENER )
            , getLongAttribute( Statistics::NUM_TIGHTENINGS_FROM_ROWS ) );

    printf( "\t\tNumber of explicit basis matrices examined by row tightener: %llu. Consequent tightenings: %llu\n"
            , getLongAttribute( Statistics::NUM_BOUND_TIGHTENINGS_ON_EXPLICIT_BASIS )
            , getLongAttribute( Statistics::NUM_TIGHTENINGS_FROM_EXPLICIT_BASIS ) );

    printf( "\t\tNumber of bound tightening rounds on the entire constraint matrix: %llu. "
            "Consequent tightenings: %llu\n"
            , getLongAttribute( Statistics::NUM_BOUND_TIGHTENINGS_ON_CONSTRAINT_MATRIX )
            , getLongAttribute( Statistics::NUM_TIGHTENINGS_FROM_CONSTRAINT_MATRIX ) );

    printf( "\t\tNumber of bound notifications sent to PL constraints: %llu. Tightenings proposed: %llu\n"
            , getLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS )
            , getLongAttribute( Statistics::NUM_BOUNDS_PROPOSED_BY_PL_CONSTRAINTS ) );

    printf( "\t--- Basis Factorization statistics ---\n" );
    printf( "\tNumber of basis refactorizations: %llu\n",
            getLongAttribute( Statistics::NUM_BASIS_REFACTORIZATIONS ) );

    unsigned long long pseNumIterations = getLongAttribute( Statistics::PSE_NUM_ITERATIONS );
    unsigned long long pseNumResetReferenceSpace =
        getLongAttribute( Statistics::PSE_NUM_RESET_REFERENCE_SPACE );
    printf( "\t--- Projected Steepest Edge Statistics ---\n" );
    printf( "\tNumber of iterations: %llu.\n", pseNumIterations );
    printf( "\tNumber of resets to reference space: %llu. Avg. iterations per reset: %u\n"
            , pseNumResetReferenceSpace
            , pseNumResetReferenceSpace > 0 ?
            (unsigned)((double)pseNumIterations / pseNumResetReferenceSpace) : 0 );

    printf( "\t--- SBT ---\n" );
    printf( "\tNumber of tightened bounds: %llu\n",
            getLongAttribute( Statistics::NUM_TIGHTENINGS_FROM_SYMBOLIC_BOUND_TIGHTENING ) );
}

unsigned long long Statistics::getTotalTimeInMicro() const
{
    return TimeUtils::timePassed( _startTime, TimeUtils::sampleMicro() );
}

unsigned Statistics::getAveragePivotTimeInMicro() const
{
    if ( _longAttributes[NUM_TABLEAU_PIVOTS] == 0 )
        return 0;

    return  _longAttributes[TIME_PIVOTS_MICRO] /
        _longAttributes[NUM_TABLEAU_PIVOTS];
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
    if ( _longAttributes[NUM_MAIN_LOOP_ITERATIONS] >= iteration )
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
