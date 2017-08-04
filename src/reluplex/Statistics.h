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

class Statistics
{
public:
    Statistics();

    /*
      Print the current statistics.
    */
    void print();

    /*
      Engine related statistics.
    */
    void incNumMainLoopIterations();
    void incNumSimplexSteps();
    void addTimeSimplexSteps( unsigned long long time );
    void incNumConstraintFixingSteps();
    unsigned long long getNumMainLoopIterations() const;
    void setNumPlConstraints( unsigned numberOfConstraints );
    void setNumActivePlConstraints( unsigned numberOfConstraints );

    /*
      Tableau related statistics.
    */
    void incNumTableauPivots();
    void incNumTableauBoundHopping();
    void incNumTableauDegeneratePivots();
    void incNumTableauDegeneratePivotsByRequest();

    /*
      Smt core related statistics.
    */
    void setCurrentStackDepth( unsigned depth );
    void incNumSplits();
    void incNumPops();

    /*
      Bound tightener related statistics.
    */
    void incNumTightenedBounds();
    void incNumRowsExaminedByTightener();

private:
    // Number of iterations of the main loop
    unsigned long long _numMainLoopIterations;

    // Number of piecewise linear constraints (active, total)
    unsigned _numPlConstraints;
    unsigned _numActivePlConstraints;

    // Number of simplex steps, i.e. pivots (including degenerate
    // pivots), performed by the main loop
    unsigned long long _numSimplexSteps;

    // Total time spent on performing simplex steps, in milliseconds
    unsigned long long _timeSimplexStepsMilli;

    // Number of constraint fixing steps, e.g. ReLU corrections,
    // performed by the main loop
    unsigned long long _numConstraintFixingSteps;

    // Current stack depth in the SMT core
    unsigned _currentStackDepth;

    // Total number of splits so far
    unsigned _numSplits;

    // Total number of pops so far
    unsigned _numPops;

    // Total number of tableau pivot operations performed, both
    // degenerate and non-degenerate
    unsigned long long _numTableauPivots;

    // Total number of degenerate tableau pivot operations performed
    unsigned long long _numTableauDegeneratePivots;

    // Total number of degenerate tableau pivot operations performed
    // by explicit request
    unsigned long long _numTableauDegeneratePivotsByRequest;

    // Total number of times a non-basic variable hopped to its
    // opposite bound.
    unsigned long long _numTableauBoundHopping;

    // Number of iterations of the main loop
    unsigned long long _numTightenedBounds;

    // Number of rows examined by the bound tightener
    unsigned long long _numRowsExaminedByTightener;

    // Printing helpers
    double printPercents( unsigned long long part, unsigned long long total ) const;
    double printAverage( unsigned long long part, unsigned long long total ) const;
};

#endif // __Statistics_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
