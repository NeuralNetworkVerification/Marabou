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
#include "Map.h"
#include "Set.h"
#include "Statistics.h"

class BasisFactorization;
class Equation;
class PiecewiseLinearCaseSplit;
class TableauState;

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
      Set which variable will enter the basis
    */
    void setEnteringVariable( unsigned nonBasic );

    // FOR TESTING ONLY
    void setLeavingVariable( unsigned basic );

    /*
      Set the values of the right hand side vector, b, of size m.
      Set either the whole vector or a specific entry
    */
    void setRightHandSide( const double *b );
    void setRightHandSide( unsigned index, double value );

    // /*
    //   Get right hand side vector.
    // */
    // const double *getRightHandSide() const;

    // /*
    //   Compute right hand side = inv(B) * b.
    // */
    // void computeRightHandSide();

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
      A method for adding an additional equation to the tableau. The
      auxiliary variable in this equation needs to be a fresh variable.
    */
    void addEquation( const Equation &equation );

    /*
      Get the Tableau's dimensions.
    */
    unsigned getM() const;
    unsigned getN() const;

    /*
      Get the assignment of a variable, either basic or non-basic
    */
    double getValue( unsigned variable );

    /*
      Given an index of a non-basic variable in the range [0,n-m),
      return the original variable that it corresponds to.
    */
    unsigned nonBasicIndexToVariable( unsigned index ) const;

    /*
      Given a variable, returns the index of that variable. The result
      is in range [0,m) if the variable is basic, or in range [0,n-m)
      if the variable is non-basic.
    */
    unsigned variableToIndex( unsigned index ) const;

    /*
      Set the lower/upper bounds for a variable. These functions are
      meant to be used as part of the initialization of the tableau.
    */
    void setLowerBound( unsigned variable, double value );
    void setUpperBound( unsigned variable, double value );

    /*
      Get the lower/upper bounds for a variable.
    */
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    /*
      Recomputes bound valid status for all variables.
    */
    void checkBoundsValid();

    /*
      Sets bound valid flag to false if bounds are invalid
      on the given variable.
    */
    void checkBoundsValid( unsigned variable );

    /*
      Returns whether any variable's bounds are invalid.
    */
    bool allBoundsValid() const;

    /*
      Tighten the lower/upper bound for a variable. These functions
      are meant to be used during the solution process, when a tighter
      bound has been discovered.
    */
    void tightenLowerBound( unsigned variable, double value );
    void tightenUpperBound( unsigned variable, double value );

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
    bool eligibleForEntry( unsigned nonBasic ) const;
    unsigned getEnteringVariable() const;
    bool nonBasicCanIncrease( unsigned nonBasic ) const;
    bool nonBasicCanDecrease( unsigned nonBasic ) const;

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
      Performs a degenerate pivot: just switches the entering and
      leaving variable. The leaving variable is required to be within
      bounds, so that is remains within bounds as a non-basic
      variable. Assignment values are unchanged (and the assignment is
      remains valid).
     */
    void performDegeneratePivot( unsigned entering, unsigned leaving );

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
      Set the assignment of a non-basic variable to a given value.
     */
    void setNonBasicAssignment( unsigned variable, double value );

    /*
      Compute the cost function (for all variables).
    */
    void computeCostFunction();

    /*
      Get a list of non-basic variables eligible for entry into the
      basis, i.e. variables that can be changed in a way that would
      reduce the cost value.
    */
    void getEntryCandidates( List<unsigned> &candidates ) const;

    /*
      Cost-calculating functions.
    */
    void computeBasicCosts();
    void computeReducedCost( unsigned nonBasic );
    void computeReducedCosts();

    /*
      Compute the multipliers for a given list of basic costs. If the
      costs are not specified as input, the function uses the basic
      costs computed by computeBasicCosts().
    */
    void computeMultipliers();
    void computeMultipliers( double *rowCoefficients );

    /*
      Access the cost function.
    */
    const double *getCostFunction() const;
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
    void dumpEquations();

    /*
      Extract a the index row from the tableau.
    */
    void getTableauRow( unsigned index, TableauRow *row );

    /*
      Store and restore the Tableau's state. Needed for case splitting
      and backtracking. The stored elements are the current:

      - Tableau dimensions
      - The current matrix A
      - Lower and upper bounds
      - Basic variables
      - Basic and non-basic assignments
      - The current indexing
      - The current basis
    */
    void storeState( TableauState &state ) const;
    void restoreState( const TableauState &state );

    /*
      Register or unregister to watch a variable.
    */
    void registerToWatchVariable( VariableWatcher *watcher, unsigned variable );
    void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable );

    /*
      Notify all watchers of the given variable of a value update,
      or of changes to its bounds.
    */
    void notifyVariableValue( unsigned variable, double value );
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Apply the bound tightenins and equations from the given split.
    */
    void applySplit( const PiecewiseLinearCaseSplit &split );

    /*
      Have the Tableau start reporting statistics.
     */
    void setStatistics( Statistics *statistics );

    void useSteepestEdge( bool flag );

    const double *getSteepestEdgeGamma() const;

    /*
      Update gamma array during a pivot
    */
    void updateGamma();

private:
    typedef List<VariableWatcher *> VariableWatchers;
    Map<unsigned, VariableWatchers> _variableToWatchers;

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
      Working memory for computing the current right hand side.
    */
    double *_rhs;

    /*
      A unit vector of size m
    */
    double *_unitVector;

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
      The assignment of the non basic variables.
    */
    double *_nonBasicAssignment;

    /*
      Upper and lower bounds for all variables
    */
    double *_lowerBounds;
    double *_upperBounds;

    /*
      Whether all variables have valid bounds (l <= u).
    */
    bool _boundsValid;

    /*
      The current assignment for the basic variables
    */
    double *_basicAssignment;

    /*
      Status of the current assignment
    */
    AssignmentStatus _basicAssignmentStatus;

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
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Flag for whether or not steepest edge is used.
    */
    bool _usingSteepestEdge;

    /*
      Array of gamma values for steepest edge pivot selection. Must be updated with
      each pivot.
     */
    double *_steepestEdgeGamma;

    /*
       Working variables for updating gamma
    */
    double *_alpha;
    double *_nu;
    double *_work;

    /*
      Free all allocated memory.
    */
    void freeMemoryIfNeeded();

    /*
      True if the basic variable is out of bounds
    */
    bool basicOutOfBounds( unsigned basic ) const;
    bool basicTooHigh( unsigned basic ) const;
    bool basicTooLow( unsigned basic ) const;

    /*
      Resize the relevant data structures to add a new row to the tableau.
    */
    void addRow();

    /*
      Initialize gamma array at tableau initialization for steepest edge
      pivot selection
    */
    void initializeGamma();

    /*
       Helper function to compute dot product of two vectors of size m
    */
    double dotProduct( const double *a, const double *b, unsigned m );

    void printVector( const double *v, unsigned m );
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
