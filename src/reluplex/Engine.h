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
#include "DantzigsRule.h"
#include "Map.h"
#include "SmtCore.h"

class PiecewiseLinearConstraint;
class InputQuery;

class Engine
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
      Get the set of all variables participating in active piecewise
      linear constraints.
     */
    const Set<unsigned> getVarsInPlConstraints();

private:
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

    /*
      The SMT engine is in charge of case splitting.
    */
    SmtCore _smtCore;

    /*
      An auxiliary variable, used to collect the part of the
      assignment that is relevant to the PL constraints.
    */
    Map<unsigned, double> _plVarAssignment;

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
      Extract the assignment of all variables that participate in a
      piecewise linear constraint. Assumes that the tableau assignment
      has been computed.
    */
    void extractPlAssignment();

    /*
      Report the violated PL constraint to the SMT engine.
    */
    void reportPlViolation();
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
