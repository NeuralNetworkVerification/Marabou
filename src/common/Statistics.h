/*********************                                                        */
/*! \file Statistics.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Statistics_h__
#define __Statistics_h__

#include "List.h"
#include "TimeUtils.h"

class Statistics
{
public:
    Statistics();

    /*
      Print the current statistics.
    */
    void print();

    /*
      Set starting time of the main loop.
    */
    void stampStartingTime();

    /*
      Set the time preprocessing took.
    */
    void setPreprocessingTime( unsigned long long milli );

    /*
      Engine related statistics.
    */
    void incNumMainLoopIterations();
    void incNumSimplexSteps();
    void addTimeMainLoop( unsigned long long time );
    void addTimeSimplexSteps( unsigned long long time );
    void addTimeConstraintFixingSteps( unsigned long long time );
    void incNumConstraintFixingSteps();
    unsigned long long getNumMainLoopIterations() const;
    void setNumPlConstraints( unsigned numberOfConstraints );
    void setNumActivePlConstraints( unsigned numberOfConstraints );
    void setNumPlValidSplits( unsigned numberOfSplits );
    void setNumPlSMTSplits( unsigned numberOfSplits );
    void setCurrentDegradation( double degradation );
    void addTimeForValidCaseSplit( unsigned long long time );
    void addTimeForSBT( unsigned long long time );
    void addTimeForStatistics( unsigned long long time );
    void addTimeForExplicitBasisBoundTightening( unsigned long long time );
    void addTimeForConstraintMatrixBoundTightening( unsigned long long time );
    void addTimeForDegradationChecking( unsigned long long time );
    void addTimeForPrecisionRestoration( unsigned long long time );
    void addTimeForApplyingStoredTightenings( unsigned long long time );
    void incNumPrecisionRestorations();
    double getMaxDegradation() const;
    unsigned getNumPrecisionRestorations() const;
    unsigned long long getTimeSimplexStepsMicro() const;
    unsigned long long getNumConstraintFixingSteps() const;

    void addNumBoundsTightenedInSBT( unsigned bounds );
    /*
      Tableau related statistics.
    */
    void incNumTableauPivots();
    void incNumTableauBoundHopping();
    void incNumTableauDegeneratePivots();
    void incNumTableauDegeneratePivotsByRequest();
    void incNumSimplexPivotSelectionsIgnoredForStability();
    void incNumSimplexUnstablePivots();
    void incNumAddedRows();
    void incNumMergedColumns();
    void setCurrentTableauDimension( unsigned m, unsigned n );
    void addTimePivots( unsigned long long time );
    unsigned getAveragePivotTimeInMicro() const;
    unsigned long long getNumTableauPivots() const;
    unsigned long long getNumSimplexPivotSelectionsIgnoredForStability() const;
    unsigned long long getNumSimplexUnstablePivots() const;

    /*
      Smt core related statistics.
    */
    void setCurrentStackDepth( unsigned depth );
    void incNumSplits();
    void incNumPops();
    void addTimeSmtCore( unsigned long long time );
    void incNumVisitedTreeStates();
    unsigned getMaxStackDepth() const;
    unsigned getNumPops() const;
    unsigned getNumVisitedTreeStates() const;
    unsigned getNumSplits() const;
    unsigned long long getTotalTime() const;

    /*
      Bound tightening related statistics.
    */
    void incNumTightenedBounds();

    void incNumRowsExaminedByRowTightener();
    void incNumTighteningsFromRows( unsigned increment = 1 );

    void incNumBoundTighteningOnConstraintMatrix();
    void incNumTighteningsFromConstraintMatrix( unsigned increment = 1 );

    void incNumBoundTighteningsOnExplicitBasis();
    void incNumTighteningsFromExplicitBasis( unsigned increment = 1 );

    void incNumBoundNotificationsPlConstraints();
    void incNumBoundsProposedByPlConstraints();

    /*
      Projected Steepest Edge related statistics.
    */
    void pseIncNumIterations();
    void pseIncNumResetReferenceSpace();

    /*
      Preprocessor statistics.
    */
    void ppSetNumEliminatedVars( unsigned eliminatedVars );
    void ppIncNumTighteningIterations();
    void ppIncNumConstraintsRemoved();
    void ppIncNumEquationsRemoved();

    /*
      For debugging purposes
    */
    void printStartingIteration( unsigned long long iteration, String message );

private:
    // Initial timestamp
    struct timespec _startTime;

    // Preprocessing time
    unsigned long long _preprocessingTimeMicro;

    // Number of iterations of the main loop
    unsigned long long _numMainLoopIterations;

    // Number of piecewise linear constraints (active, total, and reason for split)
    unsigned _numPlConstraints;
    unsigned _numActivePlConstraints;
    unsigned _numPlValidSplits;
    unsigned _numPlSmtOriginatedSplits;

    // Degradation and restorations
    double _currentDegradation;
    double _maxDegradation;
    unsigned _numPrecisionRestorations;

    // Number of simplex steps, i.e. pivots (including degenerate
    // pivots), performed by the main loop
    unsigned long long _numSimplexSteps;

    // Total time spent on performing simplex steps, in microseconds
    unsigned long long _timeSimplexStepsMicro;

    // Total time spent in the main loop, in microseconds
    unsigned long long _timeMainLoopMicro;

    // Total time spent on performing constraint fixing steps, in microseconds
    unsigned long long _timeConstraintFixingStepsMicro;

    // Number of constraint fixing steps, e.g. ReLU corrections,
    // performed by the main loop
    unsigned long long _numConstraintFixingSteps;

    // Current and max stack depth in the SMT core
    unsigned _currentStackDepth;
    unsigned _maxStackDepth;

    // Total number of splits so far
    unsigned _numSplits;

    // Total number of pops so far
    unsigned _numPops;

    // Total number of states in the search tree visited so far
    unsigned _numVisitedTreeStates;

    // Total number of tableau pivot operations performed, both
    // degenerate and non-degenerate
    unsigned long long _numTableauPivots;

    // Total number of degenerate tableau pivot operations performed
    unsigned long long _numTableauDegeneratePivots;

    // Total number of degenerate tableau pivot operations performed
    // by explicit request
    unsigned long long _numTableauDegeneratePivotsByRequest;

    // Total time for performing pivots (both real and degenrate), in microseconds
    unsigned long long _timePivotsMicro;

    // Total number of entering/leaving variable pairs ignored because their pivot
    // element was too small
    unsigned long long _numSimplexPivotSelectionsIgnoredForStability;

    // Total number of times in which an unstable simplex pivot was performed, because
    // no better option could be found.
    unsigned long long _numSimplexUnstablePivots;

    // Total number of rows added to the tableau
    unsigned long long _numAddedRows;

    // Total number of merged columns in the tableau
    unsigned long long _numMergedColumns;

    // Current Tableau dimensions
    unsigned _currentTableauM;
    unsigned _currentTableauN;

    // Total number of times a non-basic variable hopped to its
    // opposite bound.
    unsigned long long _numTableauBoundHopping;

    // Total number of all bound tightenings preformed in the tableau.
    // This combines tightenings from all sources: rows, basis, PL constraints, etc.
    unsigned long long _numTightenedBounds;

    // Number of pivot rows examined by the row tightener, and consequent tightenings
    // proposed.
    unsigned long long _numRowsExaminedByRowTightener;
    unsigned long long _numTighteningsFromRows;

    // Number of explicit basis matrices examined by the row tightener, and consequent
    // tightenings proposed.
    unsigned long long _numBoundTighteningsOnExplicitBasis;
    unsigned long long _numTighteningsFromExplicitBasis;

    // Number of bound notifications sent to pl constraints
    unsigned long long _numBoundNotificationsToPlConstraints;

    // Number of bound tightenings proposed by the pl constraints
    unsigned long long _numBoundsProposedByPlConstraints;

    // Number of bound tightening rounds performed on the constraint matrix, and
    // consequent tightenings proposed.
    unsigned long long _numBoundTighteningsOnConstraintMatrix;
    unsigned long long _numTighteningsFromConstraintMatrix;

    // Projected steepest edge statistics
    unsigned long long _pseNumIterations;
    unsigned long long _pseNumResetReferenceSpace;

    // Preprocessor counters
    unsigned _ppNumEliminatedVars;
    unsigned _ppNumTighteningIterations;
    unsigned _ppNumConstraintsRemoved;
    unsigned _ppNumEquationsRemoved;

    // Total amount of time spent performing valid case splits
    unsigned long long _totalTimePerformingValidCaseSplitsMicro;

    unsigned long long _totalTimePerformingSBT;

    // Total amount of time handling statistics printing
    unsigned long long _totalTimeHandlingStatisticsMicro;

    // Total number of valid case splits performed so far (including in other branches
    // of the search tree, that have since been popped)
    unsigned _totalNumberOfValidCaseSplits;

    // Total amount of time spent performing explicit-basis bound tightening
    unsigned long long _totalTimeExplicitBasisBoundTighteningMicro;

    // Total amount of time spent on degradation checking
    unsigned long long _totalTimeDegradationChecking;

    // Total amount of time spent on precision restoration
    unsigned long long _totalTimePrecisionRestoration;

    // Total amount of time spent performing constraint-matrix bound tightening
    unsigned long long _totalTimeConstraintMatrixBoundTighteningMicro;

    // Total amount of time spent applying previously stored bound tightenings
    unsigned long long _totalTimeApplyingStoredTighteningsMicro;

    // Total amount of time spent within the SMT core
    unsigned long long _totalTimeSmtCoreMicro;

    // Printing helpers
    double printPercents( unsigned long long part, unsigned long long total ) const;
    double printAverage( unsigned long long part, unsigned long long total ) const;

    unsigned long long _numBoundsTightenedInSBT;
};

#endif // __Statistics_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
