/*********************                                                        */
/*! \file SatSolverWrapper.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SatSolverWrapper_h__
#define __SatSolverWrapper_h__

#include "Map.h"
#include "Set.h"

#include <cadical.hpp>

class SatSolverWrapper
{
public:
    virtual ~SatSolverWrapper() = default;

    /*
      Add valid literal to clause or zero to terminate clause.
    */
    virtual void addLiteral( int lit ) = 0;

    /*
      Add a clause to the solver
    */
    virtual void addClause( const Set<int> &clause ) = 0;

    /*
      Assume valid non zero literal for next call to 'solve'.
    */
    virtual void assume( int lit ) = 0;

    /*
      Force the default decision phase of a variable to a certain value.
     */
    virtual void phase( int lit ) = 0;

    /*
      Try to solve the current formula.
    */
    virtual int solve() = 0;

    /*
      Get value (-lit=false, lit=true) of valid non-zero literal.
    */
    virtual int val( int lit ) = 0;

    /*
      Try to flip the value of the given literal without falsifying the formula.
    */
    virtual void flip( int lit ) = 0;

    /*
      Mark as 'observed' those variables that are relevant to the theory solver.
    */
    virtual void addObservedVar( int var ) = 0;

    /*
      Get reason of valid observed literal.
    */
    virtual bool isDecision( int observedVar ) const = 0;

    /*
      Return the number of vars;
     */
    virtual int vars() = 0;

    /*
     * Forces backtracking to the given level
     */
    virtual void forceBacktrack( size_t newLevel ) = 0;

    virtual Set<int> addExternalNAPClause( const String &externalNAPClauseFilename,
                                           bool isNegated ) = 0;
};


#endif // __SatSolverWrapper_h__
