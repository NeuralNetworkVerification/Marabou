/*********************                                                        */
/*! \file CadicalWrapper.h
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

#ifndef __CadicalWrapper_h__
#define __CadicalWrapper_h__

#include "File.h"
#include "SatSolverWrapper.h"

#include <cadical.hpp>
#include <memory>


class CadicalWrapper : public SatSolverWrapper
{
public:
    /*
      Constructs and destructs the Cadical Wrapper.
     */
    explicit CadicalWrapper( CaDiCaL::ExternalPropagator *externalPropagator,
                             CaDiCaL::Terminator *terminator,
                             CaDiCaL::FixedAssignmentListener *fixedListener );
    ~CadicalWrapper() override;

    /*
      Add valid literal to clause or zero to terminate clause.
    */
    void addLiteral( int lit ) override;

    /*
      Add a clause to the solver
    */
    void addClause( const Set<int> &clause ) override;

    /*
      Assume valid non zero literal for next call to 'solve'.
    */
    void assume( int lit ) override;

    /*
      Force the default decision phase of a variable to a certain value.
     */
    void phase( int lit ) override;

    /*
      Try to solve the current formula.
    */
    int solve() override;

    /*
      Get value (-lit=false, lit=true) of valid non-zero literal.
    */
    int val( int lit ) override;

    /*
      Try to flip the value of the given literal without falsifying the formula.
    */
    void flip( int lit ) override;

    /*
      Mark as 'observed' those variables that are relevant to the theory solver.
    */
    void addObservedVar( int var ) override;

    /*
      Get reason of valid observed literal.
    */
    bool isDecision( int observedVar ) const override;

    /*
      Return the number of vars;
     */
    int vars() override;

    /*
     * Forces backtracking to the given level
     */
    void forceBacktrack( size_t newLevel ) override;

    Set<int> addExternalNAPClause( const String &externalNAPClauseFilename,
                                   bool isNegated ) override;

private:
    std::shared_ptr<CaDiCaL::Solver> _solver;
};


#endif // __CadicalWrapper_h__
