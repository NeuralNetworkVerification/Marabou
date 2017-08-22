/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Engine_h__
#define __Engine_h__

#include "AutoTableau.h"
#include "BlandsRule.h"
#include "BoundTightener.h"
#include "DantzigsRule.h"
#include "DegradationChecker.h"
#include "IEngine.h"
#include "Map.h"
#include "NestedDantzigsRule.h"
#include "ProjectedSteepestEdge.h"
#include "SmtCore.h"
#include "Statistics.h"
#include "SteepestEdge.h"
#include "InputQuery.h"

class EngineState;
class InputQuery;
class PiecewiseLinearConstraint;
class String;

class Engine : public IEngine
{
public:
    Engine();
    ~Engine();

    /*
      Attempt to find a feasible solution for the input. Returns true
      if found, false if infeasible.
    */
    bool solve();

    /*
      Process the input query and pass the needed information to the
      underlying tableau.
     */
    void processInputQuery( InputQuery &inputQuery );
    void processInputQuery( InputQuery &inputQuery, bool preprocess );

    /*
      If the query is feasiable and has been successfully solved, this
      method can be used to extract the solution.
     */
    void extractSolution( InputQuery &inputQuery );

    /*
      Methods for storing and restoring the state of the engine.
    */
    void storeState( EngineState &state ) const;
    void restoreState( const EngineState &state );

    /*
      Set the entry selection strategy.
    */
    void setEntrySelectionStrategy( EntrySelectionStrategy *strategy );

private:
    /*
      Add equations and tightenings from a split.
    */
    void applySplit( const PiecewiseLinearCaseSplit &split );

    /*
      Collect and print various statistics.
    */
    Statistics _statistics;

    /*
      The tableau object maintains the equations, assignments and bounds.
    */
    AutoTableau _tableau;

    /*
      The existing piecewise-linear constraints.
    */
    List<PiecewiseLinearConstraint *> _plConstraints;

    /*
      Piecewise linear constraints that are currently violated.
    */
    List<PiecewiseLinearConstraint *>_violatedPlConstraints;

    /*
      A single, violated PL constraint, selected for fixing.
    */
    PiecewiseLinearConstraint *_plConstraintToFix;

	/*
	  Preprocessed InputQuery
	*/
	InputQuery _preprocessedQuery;

    /*
      Pivot selection strategies.
    */
    BlandsRule _blandsRule;
    DantzigsRule _dantzigsRule;
    NestedDantzigsRule _nestedDantzigsRule;
    SteepestEdgeRule _steepestEdgeRule;
    ProjectedSteepestEdgeRule _projectedSteepestEdgeRule;
    EntrySelectionStrategy *_activeEntryStrategy;

    /*
      Bound tightener.
    */
    BoundTightener _boundTightener;

    /*
      The SMT engine is in charge of case splitting.
    */
    SmtCore _smtCore;

    /*
      Number of pl constraints disabled by valid splits.
    */
    unsigned _numPlConstraintsDisabledByValidSplits;

    /*
      Degradation checker.
    */
    DegradationChecker _degradationChecker;

    /*
      Perform a simplex step: compute the cost function, pick the
      entering and leaving variables and perform a pivot. Return false
      if the problem is discovered to be unsat.
    */
    bool performSimplexStep();

    /*
      Attempt to fix one of the violated pl constraints.
    */
    void fixViolatedPlConstraintIfPossible();

    /*
      Return true iff all variables are within bounds.
     */
    bool allVarsWithinBounds() const;

    /*
      Collect all violated piecewise linear constraints.
    */
    void collectViolatedPlConstraints();

    /*
      Return true iff all piecewise linear constraints hold.
    */
    bool allPlConstraintsHold();

    /*
      Select a currently-violated LP constraint for fixing
    */
    void selectViolatedPlConstraint();

    /*
      Report the violated PL constraint to the SMT engine.
    */
    void reportPlViolation();

    /*
      Apply any bound tightenings entailed by the constraints.
    */
    void applyAllConstraintTightenings();

    /*
      Apply all valid case splits proposed by the constraints.
    */
    void applyAllValidConstraintCaseSplits();
    void applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint );

    /*
      Update statitstics, print them if needed.
    */
    void mainLoopStatistics();

    /*
      Check the current degradation, if needed.
    */
    void checkDegradation();

    static void log( const String &message );

};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
