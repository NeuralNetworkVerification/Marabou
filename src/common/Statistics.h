/*********************                                                        */
/*! \file Statistics.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Statistics_h__
#define __Statistics_h__

#include "List.h"
#include "TimeUtils.h"

class Statistics
{
public:
    Statistics();

    enum StatisticsUnsignedAttr
    {
     // Number of piecewise linear constraints (active, total, and
     // reason for split)
     NUM_PL_CONSTRAINTS,
     NUM_ACTIVE_PL_CONSTRAINTS,
     NUM_PL_VALID_SPLITS,
     NUM_PL_SMT_ORIGINATED_SPLITS,

     // Precision restoration
     NUM_PRECISION_RESTORATIONS,

     // Current and max stack depth in the SMT core
     CURRENT_DECISION_LEVEL,
     MAX_DECISION_LEVEL,

     // Total number of splits so far
     NUM_SPLITS,

     // Total number of pops so far
     NUM_POPS,

     // Total number of states in the search tree visited so far
     NUM_VISITED_TREE_STATES,

     // Current Tableau dimensions
     CURRENT_TABLEAU_M,
     CURRENT_TABLEAU_N,

     // Preprocessor counters
     PP_NUM_ELIMINATED_VARS,
     PP_NUM_TIGHTENING_ITERATIONS,
     PP_NUM_CONSTRAINTS_REMOVED,
     PP_NUM_EQUATIONS_REMOVED,

     // Total number of valid case splits performed so far (including in other
     // branches of the search tree, that have since been popped)
     TOTAL_NUMBER_OF_VALID_CASE_SPLITS,
    };

    enum StatisticsLongAttr
    {
     // Preprocessing time
     PREPROCESSING_TIME_MICRO,

     // Number of iterations of the main loop
     NUM_MAIN_LOOP_ITERATIONS,

     // Number of simplex steps, i.e. pivots (including degenerate
     // pivots), performed by the main loop
     NUM_SIMPLEX_STEPS,

     // Total time spent on performing simplex steps, in microseconds
     TIME_SIMPLEX_STEPS_MICRO,

     // Total time spent in the main loop, in microseconds
     TIME_MAIN_LOOP_MICRO,

     // Total time spent on performing constraint fixing steps, in microseconds
     TIME_CONSTRAINT_FIXING_STEPS_MICRO,

     // performed by the main loop
     NUM_CONSTRAINT_FIXING_STEPS,

     // degenerate and non-degenerate
     NUM_TABLEAU_PIVOTS,

     // Total number of degenerate tableau pivot operations performed
     NUM_TABLEAU_DEGENERATE_PIVOTS,

     // by explicit request
     NUM_TABLEAU_DEGENERATE_PIVOTS_BY_REQUEST,

     // Total time for performing pivots (both real and degenrate), in microseconds
     TIME_PIVOTS_MICRO,

     // element was too small
     NUM_SIMPLEX_PIVOT_SELECTIONS_IGNORED_FOR_STABILITY,

     // no better option could be found.
     NUM_SIMPLEX_UNSTABLE_PIVOTS,

     // Total number of rows added to the tableau
     NUM_ADDED_ROWS,

     // Total number of merged columns in the tableau
     NUM_MERGED_COLUMNS,

     // opposite bound.
     NUM_TABLEAU_BOUND_HOPPING,

     // This combines tightenings from all sources: rows, basis, PL constraints, etc.
     NUM_TIGHTENED_BOUNDS,

     // The number of bounds tightened via symbolic bound tightening
     NUM_TIGHTENINGS_FROM_SYMBOLIC_BOUND_TIGHTENING,

     // Number of pivot rows examined by the row tightener, and consequent
     // tightenings proposed.
     NUM_ROWS_EXAMINED_BY_ROW_TIGHTENER,
     NUM_TIGHTENINGS_FROM_ROWS,

     // Number of explicit basis matrices examined by the row tightener, and
     // consequent tightenings proposed.
     NUM_BOUND_TIGHTENINGS_ON_EXPLICIT_BASIS,
     NUM_TIGHTENINGS_FROM_EXPLICIT_BASIS,

     // Number of bound notifications sent to pl constraints
     NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS,

     // Number of bound notification send to transcendental constraints
     NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS,

     // Number of bound tightenings proposed by the pl constraints
     NUM_BOUNDS_PROPOSED_BY_PL_CONSTRAINTS,

     // Number of bound tightening rounds performed on the constraint matrix,
     // and consequent tightenings proposed.
     NUM_BOUND_TIGHTENINGS_ON_CONSTRAINT_MATRIX,
     NUM_TIGHTENINGS_FROM_CONSTRAINT_MATRIX,

     // Basis factorization statistics
     NUM_BASIS_REFACTORIZATIONS,

     // Projected steepest edge statistics
     PSE_NUM_ITERATIONS,
     PSE_NUM_RESET_REFERENCE_SPACE,

     // Total amount of time spent performing valid case splits
     TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO,
     TOTAL_TIME_PERFORMING_SYMBOLIC_BOUND_TIGHTENING,

     // Total amount of time handling statistics printing
     TOTAL_TIME_HANDLING_STATISTICS_MICRO,

     // Total amount of time spent performing explicit-basis bound tightening
     TOTAL_TIME_EXPLICIT_BASIS_BOUND_TIGHTENING_MICRO,

     // Total amount of time spent on degradation checking
     TOTAL_TIME_DEGRADATION_CHECKING,

     // Total amount of time spent on precision restoration
     TOTAL_TIME_PRECISION_RESTORATION,

     // Total amount of time spent performing constraint-matrix bound tightening
     TOTAL_TIME_CONSTRAINT_MATRIX_BOUND_TIGHTENING_MICRO,

     // Total amount of time spent applying previously stored bound tightenings
     TOTAL_TIME_APPLYING_STORED_TIGHTENINGS_MICRO,

     // Total amount of time spent within the SMT core
     TOTAL_TIME_SMT_CORE_MICRO,
    };

    enum StatisticsDoubleAttr
    {
     // Degradation and restorations
     CURRENT_DEGRADATION,
     MAX_DEGRADATION,
    };

    /*
      Print the current statistics.
    */
    void print();

    /*
      Set starting time of the main loop.
    */
    void stampStartingTime();

    inline void setUnsignedAttr( StatisticsUnsignedAttr attr, unsigned value )
    {
        _unsignedAttributes[attr] = value;
    }

    inline void setLongAttr( StatisticsLongAttr attr, unsigned long long value )
    {
        _longAttributes[attr] = value;
    }

    inline void setDoubleAttr( StatisticsDoubleAttr attr, double value )
    {
        _doubleAttributes[attr] = value;
    }

    inline void incUnsignedAttr( StatisticsUnsignedAttr attr, unsigned value )
    {
        _unsignedAttributes[attr] += value;
    }

    inline void incLongAttr( StatisticsLongAttr attr, unsigned long long value )
    {
        _longAttributes[attr] += value;
    }

    inline void incDoubleAttr( StatisticsDoubleAttr attr, double value )
    {
        _doubleAttributes[attr] += value;
    }

    inline unsigned getUnsignedAttr( StatisticsUnsignedAttr attr ) const
    {
        return _unsignedAttributes[attr];
    }

    inline unsigned long long  getLongAttr( StatisticsLongAttr attr ) const
    {
        return _longAttributes[attr];
    }

    inline double getDoubleAttr( StatisticsDoubleAttr attr ) const
    {
        return _doubleAttributes[attr];
    }

    /*
      Get the total runtime in microseconds.
    */
    unsigned long long getTotalTime() const;

    /*
      Report a timeout, or check whether a timeout has occurred
    */
    void timeout();
    bool hasTimedOut() const;

    /*
      For debugging purposes
    */
    void printStartingIteration( unsigned long long iteration, String message );

private:
    // Initial timestamp
    struct timespec _startTime;

    Map<StatisticsUnsignedAttr, unsigned> _unsignedAttributes;

    Map<StatisticsLongAttr, unsigned long long> _longAttributes;

    Map<StatisticsDoubleAttr, double> _doubleAttributes;

    // Whether the engine quitted with a timeout
    bool _timedOut;

    // Printing helpers
    double printPercents( unsigned long long part, unsigned long long total ) const;
    double printAverage( unsigned long long part, unsigned long long total ) const;
};

#endif // __Statistics_h__
