/*********************                                                        */
/*! \file SmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SmtCore_h__
#define __SmtCore_h__

#include "DivideStrategy.h"
#include "PLConstraintScoreTracker.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "SmtStackEntry.h"
#include "SmtState.h"
#include "Stack.h"
#include "Statistics.h"
#include "context/context.h"

#include <memory>

#define SMT_LOG( x, ... ) LOG( GlobalConfiguration::SMT_CORE_LOGGING, "SmtCore: %s\n", x )

class EngineState;
class IEngine;
class String;

using CVC4::context::Context;

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
    void initializeScoreTrackerIfNeeded( const List<PiecewiseLinearConstraint *> &plConstraints );

    /*
      Inform the SMT core that a SoI phase pattern proposal is rejected.
    */
    void reportRejectedPhasePatternProposal();

    /*
      Update the score of the constraint with the given score in the costTracker.
    */
    inline void updatePLConstraintScore( PiecewiseLinearConstraint *constraint, double score )
    {
        ASSERT( _scoreTracker != nullptr );
        _scoreTracker->updateScore( constraint, score );
    }

    /*
      Get the constraint in the score tracker with the highest score
    */
    inline PiecewiseLinearConstraint *getConstraintsWithHighestScore() const
    {
        return _scoreTracker->topUnfixed();
    }

    /*
      Inform the SMT core that a PL constraint is violated.
    */
    void reportViolatedConstraint( PiecewiseLinearConstraint *constraint );

    /*
      Get the number of times a specific PL constraint has been reported as
      violated.
    */
    unsigned getViolationCounts( PiecewiseLinearConstraint *constraint ) const;

    /*
      Reset all reported violation counts and the number of rejected SoI
      phase pattern proposal.
    */
    void resetSplitConditions();

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
         Pop _context, record statistics
     */
    void popContext();

    /*
         Push _context, record statistics
     */
    void pushContext();


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
    PiecewiseLinearConstraint *chooseViolatedConstraintForFixing(
        List<PiecewiseLinearConstraint *> &_violatedPlConstraints ) const;

    inline void setBranchingHeuristics( DivideStrategy strategy )
    {
        _branchingHeuristic = strategy;
    }

    /*
      Replay a stackEntry
    */
    void replaySmtStackEntry( SmtStackEntry *stackEntry );

    /*
      Store the current state of the SmtCore into smtState
    */
    void storeSmtState( SmtState &smtState );

    /*
      Pick the piecewise linear constraint for splitting, returns true
      if a constraint for splitting is successfully picked
    */
    bool pickSplitPLConstraint();

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
      Context for synchronizing the search.
     */
    Context &_context;
    /*
      Do we need to perform a split and on which constraint.
    */
    bool _needToSplit;
    PiecewiseLinearConstraint *_constraintForSplitting;

    /*
      Count how many times each constraint has been violated.
    */
    Map<PiecewiseLinearConstraint *, unsigned> _constraintToViolationCount;

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
      Split when some relu has been violated for this many times during the
      Reluplex procedure
    */
    unsigned _constraintViolationThreshold;

    /*
      Split when there have been this many rejected phase pattern proposal
      during the SoI-based local search.
    */
    unsigned _deepSoIRejectionThreshold;

    /*
      The strategy to pick the piecewise linear constraint to branch on.
    */
    DivideStrategy _branchingHeuristic;

    /*
      Heap to store the scores of each PLConstraint.
    */
    std::unique_ptr<PLConstraintScoreTracker> _scoreTracker;

    /*
      Number of times the phase pattern proposal has been rejected at the
      current search state.
    */
    unsigned _numRejectedPhasePatternProposal;
};

#endif // __SmtCore_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
