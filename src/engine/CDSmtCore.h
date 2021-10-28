/*********************                                                        */
/*! \file CDSmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   AleksandarZeljic, Guy Katz, Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This class implements a context-dependent SmtCore class with lazy
 ** backtracking of search state. The search state is stored lightly using the
 ** PhaseStatus enumeration in context-dependent PiecewiseLinearConstraint
 ** class. The exhaustive search relies on correct implementation of the
 ** isFeasible()/nextFeasibleCase() methods in PiecewiseLinearConstraint class.
 **
 ** TODO: Incremental frames
 **/

#ifndef __CDSmtCore_h__
#define __CDSmtCore_h__

#include "context/context.h"
#include "context/cdlist.h"
#include "Options.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "Stack.h"
#include "Statistics.h"
#include "TrailEntry.h"

#define SMT_LOG( x, ... ) LOG( GlobalConfiguration::SMT_CORE_LOGGING, "CDSmtCore: %s\n", x )

class EngineState;
class Engine;
class String;

class CDSmtCore
{
public:
    CDSmtCore( IEngine *engine, CVC4::context::Context &context );
    ~CDSmtCore();

    /*
      Clear the stack.
    */
    void freeMemory();

    /*
      Inform the SMT core that a PL constraint is violated.
    */
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
      Returns true iff the SMT core wants to perform a case split.
    */
    bool needToSplit() const;

    /*
      Decide a case split to apply, according to the constraint marked for
      splitting. Update bounds, add equations and update the stack.
    */
    void decide();

    /*
     * decideSplit - choose a phase of PiecewiseLinearConstraint's alternativeCases to decide.
     */
    void decideSplit( PiecewiseLinearConstraint *constraint );

    /*
     * Push TrailEntry representing the decision onto the trail. Push the decided PiecewiseLinearCaseSplit to the engine.
     */
    void pushDecision( PiecewiseLinearConstraint *constraint,  PhaseStatus decision );

    /*
      Let the smt core know of an implied valid case split that was discovered.
    */
    void implyValidSplit( PiecewiseLinearCaseSplit &validSplit );

    /*
      Let the smt core trail know of an implied valid case split that was discovered.
    */
    void pushImplication( PiecewiseLinearConstraint *constraint, PhaseStatus phase );

    /*
        Pushes trail element onto trail, handles decision book-keeping and
        applies the case split to the engine.
     */
    void applyTrailEntry( TrailEntry &te, bool isDecision = false );

    /*
      Backtrack the search, by popping stacks with no alternatives, and perform
      a decision or an implication as needed.
    */
    bool backtrackAndContinue();

    /*
      Pop a stack frame. Return true if successful, false if the stack is empty.
    */
    bool popSplit();

    /*
      Pop a context level, lazily backtracking trail, bounds, etc. Return true
      if successful, false if the stack is empty.
    */
    bool popDecisionLevel( TrailEntry &lastDecision );

    /*
      The current stack depth.
    */
    unsigned getDecisionLevel() const;

    /*
      Return a list of all splits performed so far, both SMT-originating and valid ones,
      in the correct order.
    */
    void allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const;

    /*
      Get trail begin iterator.
    */
    CVC4::context::CDList<TrailEntry>::const_iterator trailBegin() const
    {
        return _trail.begin();
    };

    /*
      Get trail end iterator.
    */
    CVC4::context::CDList<TrailEntry>::const_iterator trailEnd() const
    {
        return _trail.end();
    };

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
      Pick the piecewise linear constraint for splitting, returns true
      if a constraint for splitting is successfully picked
    */
    bool pickSplitPLConstraint();

    /*
     * For testing purposes
     */
    PiecewiseLinearCaseSplit getDecision( unsigned decisionLevel ) const;

    void reset();

    /*
      For debugging purposes only - store a correct possible solution
      TODO: Create a user interface for this
    */
    void storeDebuggingSolution( const Map<unsigned, double> &debuggingSolution );
    bool checkSkewFromDebuggingSolution();
    bool splitAllowsStoredSolution( const PiecewiseLinearCaseSplit &split, String &error ) const;
    void interruptIfCompliantWithDebugSolution();
private:
    /*
      A stack entry consists of the engine state before the split,
      the active split, the alternative splits (in case of backtrack),
      and also any implied splits that were discovered subsequently.
    */
    struct StackEntry
    {
    public:
        PiecewiseLinearCaseSplit _activeSplit;
        PiecewiseLinearConstraint *_sourceConstraint;
        List<PiecewiseLinearCaseSplit> _impliedValidSplits;
        List<PiecewiseLinearCaseSplit> _alternativeSplits;
        EngineState *_engineState;
    };



    /*
      Valid splits that were implied by level 0 of the stack.
    */
    List<PiecewiseLinearCaseSplit> _impliedValidSplitsAtRoot;

    /*
      Collect and print various statistics.
    */
    Statistics *_statistics;


    /*
      CVC4 Context, constructed in Engine
    */
    CVC4::context::Context& _context;

    /*
      The case-split stack.
    */
    //List<StackEntry *> _stack;

    /*
      Trail is context dependent and contains all the asserted PWLCaseSplits. 
      TODO: Abstract from PWLCaseSplits to Literals
    */
    CVC4::context::CDList<TrailEntry> _trail;

    /*
     * _decisions point to the decision at the beginning of each decision level
     */
    CVC4::context::CDList<TrailEntry> _decisions;

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
      Split when some relu has been violated for this many times
    */
    unsigned _constraintViolationThreshold;

    /*
      Helper function to establish equivalence between trail and stack information
     */
    bool checkStackTrailEquivalence();
};

#endif // __CDSmtCore_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
