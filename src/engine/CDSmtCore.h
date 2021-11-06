/*********************                                                        */
/*! \file CDSmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, AleksandarZeljic,  Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** The CDSmtCore class implements a context-dependent SmtCore class.
 ** The CDSmtCore distinguishes between: **decisions** and **implications**.
 **
 ** Decision is a case of PiecewiseLinearConstraint asserted on the trail.
 ** Decision implies that the PiecewiseLinearConstraint has at least one other
 ** feasible case. Decisions represent nodes/search-states in the search tree.
 **
 ** Implication is a case of PiecewiseLinearConstraint asserted on the trail.
 ** Implication is the last feasible case of a PiecewiseLinearConstraint.
 **
 ** Case splitting on a PiecewiseLinearConstraint performs a decision.
 ** Fixing  a case of a PiecewiseLinearConstraint (e.g., via bound propagation)
 ** performs an implication.
 **
 ** The overall search state is stored in a distributed way: the CDSmtCore
 ** stores the current search state while the PiecewiseLinearConstraints'
 ** infeasible cases enumerates all the explored states w.r.t to the
 ** chronological order on the _trail.
 **
 ** _trail is a chronological list of cases of PiecewiseLinearConstraints
 ** asserted to hold (represented using TrailEntry) - both decisions and
 ** implications. _decisions is a chronological list of decisions stored on the
 ** trail. _trail and _decisions are both context dependent and will synchronize
 ** in unison with the context object.
 **
 ** When a search state is found to be infeasible, CDSmtCore backtracks lazily
 ** to the last decision and continues the search.
 **
 ** Context management is done automatically when a new decision is pushed and a
 ** decision level is popped. Popping a decision will cause all related context
 ** dependent data structures to backtrack in sync with the _trail. This will
 ** effectively backtrack the entire search state via PiecewiseLinearConstraints
 ** and _boundManager. The only exception is the state of basic/non-basic
 ** variables in the tableau, which may need to be recalculated. However, no
 ** additional memory overhead is incurred in this process.
 **
 ** Implementation relies on:
 **
 ** * _context is a unique Context object from which all the context-dependent
 ** structures are obtained.
 **
 ** * PiecewiseLinearConstraint class stores its search state in a
 ** context-dependent manner and exposes it using nextFeasibleCase() and
 ** markInfeasible() methods.
 **
 ** * Using BoundManager class to store bounds in a context-dependent manner
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
       Push TrailEntry representing the decision onto the trail. 
     */
    void pushDecision( PiecewiseLinearConstraint *constraint,  PhaseStatus decision );

    /*
      Inform SmtCore of an implied (formerly valid) case split that was discovered.
    */
    void pushImplication( PiecewiseLinearConstraint *constraint );

    /*
        Pushes trail entry onto trail, handles decision book-keeping and
        Update bounds and add equations to the engine.
     */
    void applyTrailEntry( TrailEntry &te, bool isDecision = false );

    /*
      Decide and apply a case split using the constraint marked for splitting.
    */
    void decide();

    /*
      Decide a constraint's feasible case. Update bounds, add equations
      and trail.
     */
    void decideSplit( PiecewiseLinearConstraint *constraint );

    /*
       Backtracks fully explored decisions and stores the last (feasible decision
     */
    bool backtrackToFeasibleDecision( TrailEntry &feasibleDecision );

    /*
      Return to a feasible state and resume search by asserting the next case
      (as either a decision or implication)
    */
    bool backtrackAndContinueSearch();

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
      Collect and print various statistics.
    */
    Statistics *_statistics;

    /*
      CVC4 Context, constructed in Engine
    */
    CVC4::context::Context& _context;

    /*
      Trail is context dependent and contains all the asserted PWLCaseSplits
    */
    CVC4::context::CDList<TrailEntry> _trail;

    /*
     * _decisions stores the decision TrailEntries
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
      Split when some relu has been violated for this many times
    */
    unsigned _constraintViolationThreshold;

};

#endif // __CDSmtCore_h__

