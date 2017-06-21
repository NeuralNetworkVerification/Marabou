/*********************                                                        */
/*! \file Tableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Tableau_h__
#define __Tableau_h__

#include "ITableau.h"
#include "Set.h"

class BasisFactorization;

class Tableau : public ITableau
{
public:
    enum BasicStatus {
        BELOW_LB = 0,
        AT_LB,
        BETWEEN,
        AT_UB,
        ABOVE_UB,
    };

    enum AssignmentStatus {
        ASSIGNMENT_INVALID,
        ASSIGNMENT_VALID,
    };

    Tableau();
    ~Tableau();

    /*
      Allocate space for the various data structures
      n: total number of variables
      m: number of constraints (rows)
    */
    void setDimensions( unsigned m, unsigned n );

    /*
      Set the value of a specific entry in the tableau
    */
    void setEntryValue( unsigned row, unsigned column, double value );

    /*
      Set the values of the right hand side vector, b, of size m.
      Set either the whole vector or a specific entry
    */
    void setRightHandSide( const double *b );
    void setRightHandSide( unsigned index, double value );

    /*
      Mark a variable as basic in the initial basis
     */
    void markAsBasic( unsigned variable );

    /*
      Initialize the tableau matrices (_B and _AN) according to the
      initial set of basic variables. Assign all non-basic variables
      to lower bounds and computes the assignment
    */
    void initializeTableau();

    /*
      Get the assignment of a variable, either basic or non-basic
    */
    double getValue( unsigned variable );

    /*
      Set the lower/upper bounds for a variable
    */
    void setLowerBound( unsigned variable, double value );
    void setUpperBound( unsigned variable, double value );

    /*
      Return the current status of the basic variable
    */
    unsigned getBasicStatus( unsigned basic );

    /*
      True if there exists some out-of-bounds basic
    */
    bool existsBasicOutOfBounds() const;

    /*
      Compute the status of the basic variable based on current assignment
    */
    void computeBasicStatus();
    void computeBasicStatus( unsigned basic );

    /*
      Picks the entering variable.
    */
    bool pickEnteringVariable();
    bool eligibleForEntry( unsigned nonBasic );
    unsigned getEnteringVariable() const;

    /*
      Pick the leaving variable according to the entering variable.
      d is the column vector for the entering variable (length m)
    */
    void pickLeavingVariable();
    void pickLeavingVariable( double *d );
    unsigned getLeavingVariable() const;
    double getChangeRatio() const;

    /*
      Performs the pivot operation after the entering and leaving
      variables have been selected
    */
    void performPivot();

    /*
      Calculate the ratio constraint for the entering variable
      imposed by a basic variable.
      Coefficient is the relevant coefficient in the tableau.
      Decrease is true iff the entering variable is decreasing.
    */
    double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease );

    /*
      True iff the variable is basic
    */
    bool isBasic( unsigned variable ) const;

    /*
      Compute the cost function
    */
    void computeCostFunction();
    const double *getCostFunction();
    void dumpCostFunction() const;

    /*
      Compute _d = inv(B) * a
    */
    void computeD();

    /*
      Compute the basic assignment
    */
    void computeAssignment();

    /*
      Dump the tableau (debug)
    */
    void dump() const;
    void dumpAssignment();

private:
    /*
      The dimensions of matrix A
    */
    unsigned _n;
    unsigned _m;

    /*
      The matrix
    */
    double *_A;

    /*
      The basis matrix, which is a subset of A's columns
    */
    double *_B;

    /*
      The non-basic matrix, which is a subset of A's columns
    */
    double *_AN;

    /*
      A single column matrix from A
    */
    double *_a;

    /*
      Used to compute inv(B)*a
    */
    double *_d;

    /*
      The right hand side vector of Ax = b
    */
    double *_b;

    /*
      The current factorization of the basis
    */
    BasisFactorization *_basisFactorization;

    /*
      The cost function and auxiliary variable for computing it
    */
    double *_costFunction;
    double *_basicCosts;
    double *_multipliers;

    /*
      Mapping between basic variables and indices (length m)
    */
    unsigned *_basicIndexToVariable;

    /*
      Mapping between non-basic variables and indices (length n - m)
    */
    unsigned *_nonBasicIndexToVariable;

    /*
      Mapping from variable to index, either basic or non-basic
    */
    unsigned *_variableToIndex;

    /*
      The set of current basic variables
    */
    Set<unsigned> _basicVariables;

    /*
      The position of the non basic variables.
      True at upper bound, false at lower bound
    */
    bool *_nonBasicAtUpper;

    /*
      Upper and lower bounds for all variables
    */
    double *_lowerBounds;
    double *_upperBounds;

    /*
      The current assignment for the basic variables
    */
    double *_assignment;

    /*
      Status of the current assignment
    */
    AssignmentStatus _assignmentStatus;

    /*
      The current status of the basic variabels
    */
    unsigned *_basicStatus;

    /*
      A non-basic variable chosen to become basic in this iteration
    */
    unsigned _enteringVariable;

    /*
      A basic variable chosen to become non-basic in this iteration
    */
    unsigned _leavingVariable;

    /*
      The amount by which the entering variable should change in this
      pivot step
    */
    double _changeRatio;

    /*
      True if the leaving variable increases, false otherwise
    */
    bool _leavingVariableIncreases;

    /*
      Helper functions for calculating the cost
    */
    void computeBasicCosts();
    void computeMultipliers();
    void computeReducedCosts();

    /*
      True if the basic variable is out of bounds
    */
    bool basicOutOfBounds( unsigned basic ) const;
    bool basicTooHigh( unsigned basic ) const;
    bool basicTooLow( unsigned basic ) const;
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
