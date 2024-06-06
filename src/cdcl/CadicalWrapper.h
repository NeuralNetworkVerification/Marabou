#ifndef MARABOU_CADICALWRAPPER_H
#define MARABOU_CADICALWRAPPER_H

#include "SatSolverWrapper.h"

#include <cadical.hpp>
#include <memory>


class CadicalWrapper : SatSolverWrapper
{
public:
    /*
      Constructs the Cadical Wrapper.
     */
    CadicalWrapper();

    /*
      Add valid literal to clause or zero to terminate clause.
    */
    void addLiteral( int lit ) override;

    /*
      Add a clause to the solver
    */
    void addClause( const Set<int> &clause ) override;

    /*
     check if the formula is already inconsistent
    */
    bool inconsistent() override;

    /*
      Assume valid non zero literal for next call to 'solve'.
    */
    void assume( int lit ) override;

    /*
      Try to solve the current formula.
    */
    int solve() override;

    /*
      Get value (-lit=false, lit=true) of valid non-zero literal.
    */
    int val( int lit ) override;

    /*
      Get values of all of the literals.
    */
    Map<int, int> getModel();

    /*
      Try to flip the value of the given literal without falsifying the formula.
    */
    void flip( int lit ) override;

    /*
      Add call-back which allows to learn, propagate and backtrack based on external constraints.
    */
    void connectTheorySolver() override;

    /*
      Disconnect the theory solver, resets all the observed variables.
    */
    void disconnectTheorySolver() override;

    /*
      Mark as 'observed' those variables that are relevant to the theory solver.
    */
    void addObservedVar( int var ) override;

    /*
      Removes the 'observed' flag from the given variable.
    */
    void removeObservedVar( int var ) override;

    /*
      Get reason of valid observed literal.
    */
    bool isDecision( int observedVar ) override;

private:
    std::shared_ptr<CaDiCaL::Solver> d_solver;
    std::unique_ptr<CaDiCaL::Terminator> d_terminator;
};


#endif // MARABOU_CADICALWRAPPER_H
