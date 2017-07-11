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
    void addTimeSimplexSteps( unsigned long long time);
    void incNumConstraintFixingSteps();
    unsigned long long getNumMainLoopIterations() const;

    /*
      Smt core related statistics.
    */
    void setCurrentStackDepth( unsigned depth );
    void incNumSplits();
    void incNumPops();

private:
    // Number of iterations of the main loop
    unsigned long long _numMainLoopIterations;

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

    // Printing helpers
    unsigned long long printPercents( unsigned long long part, unsigned long long total ) const;
    unsigned long long printAverage( unsigned long long part, unsigned long long total ) const;
};

#endif // __Statistics_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
