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
#include "IEngine.h"
#include "Map.h"
#include "NestedDantzigsRule.h"
#include "SmtCore.h"
#include "Statistics.h"
#include "SteepestEdge.h"

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
    void processInputQuery( const InputQuery &inputQuery );

    /*
      If the query is feasiable and has been successfully solved, this
      method can be used to extract the solution.
     */
    void extractSolution( InputQuery &inputQuery );

    /*
      Methods for tightening lower/upper variable bounds.
    */
    void tightenLowerBound( unsigned variable, double bound );
    void tightenUpperBound( unsigned variable, double bound );

    /*
      Add a new equation to the Tableau.
    */
    void addNewEquation( const Equation &equation );

    /*
      Methods for storing and restoring the tableau.
    */
    void storeTableauState( TableauState &state ) const;
    void restoreTableauState( const TableauState &state );

    /*
      Get the tableau.
    */
    ITableau *getTableau();

private:    
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
      Pivot selection strategies.
    */
    BlandsRule _blandsRule;
    DantzigsRule _dantzigsRule;
    NestedDantzigsRule _nestedDantzigsRule;
    SteepestEdgeRule _steepestEdgeRule;
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
      Perform a simplex step: compute the cost function, pick the
      entering and leaving variables and perform a pivot. Return false
      if the problem is discovered to be unsat.
    */
    bool performSimplexStep();

    /*
      Fix one of the violated pl constraints return false if the
      problem is discovered to be unsat.
    */
    bool fixViolatedPlConstraint();

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
      Print a message
    */
    void log( const String &line ) const;
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
