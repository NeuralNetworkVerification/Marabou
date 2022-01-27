/*********************                                                        */
/*! \file SmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SmtCore_h__
#define __SmtCore_h__

#include "DivideStrategy.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "PLConstraintScoreTracker.h"
#include "SmtState.h"
#include "Stack.h"
#include "SmtStackEntry.h"
#include "Statistics.h"

#include <memory>

#define SMT_LOG( x, ... ) LOG( GlobalConfiguration::SMT_CORE_LOGGING, "SmtCore: %s\n", x )

class EngineState;
class IEngine;
class String;

class SmtCore
{
public:
    SmtCore( IEngine *engine );
    ~SmtCore();

    /*
      Clear the stack.
    */
    void freeMemory();

    /*
      Reset the SmtCore
    */
    void reset();

    /*
      Initialize the score tracker with the given list of pl constraints.
    */
    void initializeScoreTracker( const List<PiecewiseLinearConstraint *>
                                 &plConstraints );

    void reportViolatedConstraint( PiecewiseLinearConstraint *constraint );

    /*
      Get the number of times a specific PL constraint has been reported as
      violated.
    */
    unsigned getViolationCounts( PiecewiseLinearConstraint* constraint ) const;

    /*
      Reset all reported violation counts.
    */
    void resetReportedViolations();

    /*
      Update the score of the constraint with the given score in the costTracker.
    */
    inline void updatePLConstraintScore( PiecewiseLinearConstraint *constraint,
                                  double score )
    {
        ASSERT( _scoreTracker != nullptr );
        _scoreTracker->updateScore( constraint, score );
    }

    /*
      Pick the PLConstraint to branch on next.
    */
    void pickBranchingPLConstraint();

    /*
      Returns true iff the SMT core wants to perform a case split.
    */
    bool needToSplit() const;

    /*
      Perform the split according to the constraint marked for
      splitting. Update bounds, add equations and update the stack.
    */
    void performSplit();

    /*
      Pop an old split from the stack, and perform a new split as
      needed. Return true if successful, false if the stack is empty.
    */
    bool popSplit();

    /*
      The current stack depth.
    */
    unsigned getStackDepth() const;

    /*
      Let the smt core know of an implied valid case split that was discovered.
    */
    void recordImpliedValidSplit( PiecewiseLinearCaseSplit &validSplit );

    /*
      Return a list of all splits performed so far, both SMT-originating and valid ones,
      in the correct order.
    */
    void allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const;

    /*
      Have the SMT core start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

    /*
      Have the SMT core choose, among a set of violated PL constraints, which
      constraint should be repaired (without splitting)
    */
    PiecewiseLinearConstraint *chooseViolatedConstraintForFixing( List<PiecewiseLinearConstraint *> &_violatedPlConstraints ) const;

    void setConstraintViolationThreshold( unsigned threshold );

    /*
      Replay a stackEntry
    */
    void replaySmtStackEntry( SmtStackEntry *stackEntry );

    /*
      Store the current state of the SmtCore into smtState
    */
    void storeSmtState( SmtState &smtState );

    inline void setBranchingHeuristics( DivideStrategy strategy )
    {
        _branchingHeuristic = strategy;
    }

    /*
      For debugging purposes only - store a correct possible solution
    */
    void storeDebuggingSolution( const Map<unsigned, double> &debuggingSolution );
    bool checkSkewFromDebuggingSolution();
    bool splitAllowsStoredSolution( const PiecewiseLinearCaseSplit &split, String &error ) const;

private:
    /*
      Valid splits that were implied by level 0 of the stack.
    */
    List<PiecewiseLinearCaseSplit> _impliedValidSplitsAtRoot;

    /*
      Collect and print various statistics.
    */
    Statistics *_statistics;

    /*
      The case-split stack.
    */
    List<SmtStackEntry *> _stack;

    /*
      The engine.
    */
    IEngine *_engine;

    /*
      Do we need to perform a split and on which constraint.
    */
    bool _needToSplit;
    PiecewiseLinearConstraint *_constraintForSplitting;

    /*
      For debugging purposes only
    */
    Map<unsigned, double> _debuggingSolution;

    /*
      A unique ID allocated to every state that is stored, for
      debugging purposes.
    */
    unsigned _stateId;

    /*
      Split when some relu has been violated for this many times
    */
    unsigned _constraintViolationThreshold;

    /*
      The strategy to pick the piecewise linear constraint to branch on.
    */
    DivideStrategy _branchingHeuristic;

    /*
      Heap to store the scores of each PLConstraint.
    */
    std::unique_ptr<PLConstraintScoreTracker> _scoreTracker;

    /*
      Reset the score tracker unless we are using PseudoImpact branching.
    */
    void resetScoresIfNeeded();

    /*
      Set the constraint score to 0 unless we are using PseudoImpact branching.
    */
    void resetScoreIfNeeded( PiecewiseLinearConstraint *constraint );
};

#endif // __SmtCore_h__
