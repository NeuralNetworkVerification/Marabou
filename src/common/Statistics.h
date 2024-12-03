/*********************                                                        */
/*! \file Statistics.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Statistics_h__
#define __Statistics_h__

#include "List.h"
#include "Map.h"
#include "TimeUtils.h"

class Statistics
{
public:
    Statistics();

    enum StatisticsUnsignedAttribute {
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

        // Number of calls to context push and pop
        NUM_CONTEXT_PUSHES,
        NUM_CONTEXT_POPS,

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

        // Total number of delegated leaves, certified leaves and lemmas in the search tree
        NUM_CERTIFIED_LEAVES,
        NUM_DELEGATED_LEAVES,
        NUM_LEMMAS,

        // 1 if returned UNSAT and proof was certified by proof checker, 0 otherwise.
        CERTIFIED_UNSAT,
    };

    enum StatisticsLongAttribute {
        // Preprocessing time
        PREPROCESSING_TIME_MICRO,

        // Calculate output bounds time
        CALCULATE_BOUNDS_TIME_MICRO,

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

        // Number of bound notification send to nonlinear constraints
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

        // Total time heuristically updating the SoI phase pattern
        TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,

        // Number of proposed/accepted update to the SoI phase pattern.
        NUM_PROPOSED_PHASE_PATTERN_UPDATE,
        NUM_ACCEPTED_PHASE_PATTERN_UPDATE,

        // Total time obtaining the current variable assignment from the tableau.
        TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_MICRO,

        // Total time performing SoI-based local search
        TOTAL_TIME_LOCAL_SEARCH_MICRO,

        // Total time getting the SoI phase pattern
        TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,

        // Total time adding constraints to (MI)LP solver.
        TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,

        // Total time spent in context-switching
        TIME_CONTEXT_PUSH,
        TIME_CONTEXT_POP,
        TIME_CONTEXT_PUSH_HOOK,
        TIME_CONTEXT_POP_HOOK,

        // Total Certification Time
        TOTAL_CERTIFICATION_TIME,
    };

    enum StatisticsDoubleAttribute {
        // Degradation and restorations
        CURRENT_DEGRADATION,
        MAX_DEGRADATION,

        // How close we are to the minimum of the SoI (0).
        COST_OF_CURRENT_PHASE_PATTERN,
        MIN_COST_OF_PHASE_PATTERN,
    };

    /*
      Print the current statistics.
    */
    void print();

    /*
      Set starting time of the main loop.
    */
    void stampStartingTime();

    /*
      Setters for unsigned, unsigned long long, and double attributes
    */
    inline void setUnsignedAttribute( StatisticsUnsignedAttribute attr, unsigned value )
    {
        _unsignedAttributes[attr] = value;
    }

    inline void incUnsignedAttribute( StatisticsUnsignedAttribute attr )
    {
        ++_unsignedAttributes[attr];
    }

    inline void incUnsignedAttribute( StatisticsUnsignedAttribute attr, unsigned value )
    {
        _unsignedAttributes[attr] += value;
    }

    inline void setLongAttribute( StatisticsLongAttribute attr, unsigned long long value )
    {
        _longAttributes[attr] = value;
    }

    inline void incLongAttribute( StatisticsLongAttribute attr )
    {
        ++_longAttributes[attr];
    }

    inline void incLongAttribute( StatisticsLongAttribute attr, unsigned long long value )
    {
        _longAttributes[attr] += value;
    }

    inline void setDoubleAttribute( StatisticsDoubleAttribute attr, double value )
    {
        _doubleAttributes[attr] = value;
    }

    inline void incDoubleAttribute( StatisticsDoubleAttribute attr, double value )
    {
        _doubleAttributes[attr] += value;
    }

    /*
      Getters for unsigned, unsigned long long, and double attributes
    */
    inline unsigned getUnsignedAttribute( StatisticsUnsignedAttribute attr ) const
    {
        return _unsignedAttributes[attr];
    }

    inline unsigned long long getLongAttribute( StatisticsLongAttribute attr ) const
    {
        return _longAttributes[attr];
    }

    inline double getDoubleAttribute( StatisticsDoubleAttribute attr ) const
    {
        return _doubleAttributes[attr];
    }

    unsigned long long getTotalTimeInMicro() const;

    unsigned getAveragePivotTimeInMicro() const;

    /*
      Report a timeout, or check whether a timeout has occurred
    */
    void timeout();
    bool hasTimedOut() const;

    /*
      For debugging purposes
    */
    void printStartingIteration( unsigned long long iteration, String message );

    /*
      Print a long attribute in time format
    */
    void printLongAttributeAsTime( unsigned long long longAsNumber ) const;

private:
    // Initial timestamp
    struct timespec _startTime;

    Map<StatisticsUnsignedAttribute, unsigned> _unsignedAttributes;

    Map<StatisticsLongAttribute, unsigned long long> _longAttributes;

    Map<StatisticsDoubleAttribute, double> _doubleAttributes;

    // Whether the engine quitted with a timeout
    bool _timedOut;

    // Printing helpers
    double printPercents( unsigned long long part, unsigned long long total ) const;
    double printAverage( unsigned long long part, unsigned long long total ) const;
};

#endif // __Statistics_h__
