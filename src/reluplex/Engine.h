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

private:
    AutoTableau _tableau;
    List<PiecewiseLinearConstraint *> _plConstraints;
    BlandsRule _blandsRule;
    DantzigsRule _dantzigsRule;
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
