#ifndef MARABOU_SATSOLVERWRAPPER_H
#define MARABOU_SATSOLVERWRAPPER_H

#include "Map.h"
#include "Set.h"

class SatSolverWrapper
{
public:
    /*
      Add valid literal to clause or zero to terminate clause.
    */
    virtual void addLiteral( int lit ) = 0;

    /*
      Add a clause to the solver
    */
    virtual void addClause( const Set<int> &clause ) = 0;

    /*
     check if the formula is already inconsistent
    */
    virtual bool inconsistent() = 0;

    /*
      Assume valid non zero literal for next call to 'solve'.
    */
    virtual void assume( int lit ) = 0;

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
      Add call-back which allows to learn, propagate and backtrack based on external constraints.
    */
    virtual void connectTheorySolver() = 0; // TODO: add parameter representing the theory solver

    /*
      Disconnect the theory solver, resets all the observed variables.
    */
    virtual void disconnectTheorySolver() = 0;

    /*
      Mark as 'observed' those variables that are relevant to the theory solver.
    */
    virtual void addObservedVar( int var ) = 0;

    /*
      Removes the 'observed' flag from the given variable.
    */
    virtual void removeObservedVar( int var ) = 0;

    /*
      Get reason of valid observed literal.
    */
    virtual bool isDecision( int observedVar ) = 0;
};


#endif // MARABOU_SATSOLVERWRAPPER_H
