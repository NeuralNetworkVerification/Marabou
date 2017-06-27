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
    AutoTableau _tableau;
    List<PiecewiseLinearConstraint *> _plConstraints;
    BlandsRule _blandsRule;
    DantzigsRule _dantzigsRule;
    Map<unsigned, double> _plVarAssignment;

    /*
      Perform a simplex step: compute the cost function, pick the
      entering and leaving variables and perform a pivot. Return false
      iff the problem is unsat.
    */
    bool performSimplexStep();

    /*
      Return true iff all variables are within bounds.
     */
    bool allVarsWithinBounds() const;

    /*
      Return true iff all piecewise linear constraints hold.
    */
    bool allPlConstraintsHold();

    /*
      Extract the assignment of all variables that participate in a
      piecewise linear constraint. Assumes that the tableau assignment
      has been computed.
    */
    void extractPlAssignment();
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
