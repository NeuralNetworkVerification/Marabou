/*********************                                                        */
/*! \file Tableau.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Tableau_h__
#define __Tableau_h__

#include "Set.h"

class Tableau
{
public:
    enum BasicStatus {
        BELOW_LB = 0,
        AT_LB,
        BETWEEN,
        // FIXED,
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
      Initialize the tableau.
       n: total number of variables
       m: number of constraints (rows)
    */
    void setDimensions( unsigned m, unsigned n );

    /*
      Set the value of a specific entry in the tableau
    */
    void setEntryValue( unsigned row, unsigned column, double value );

    /*
      Set the values of the right hand side vector, b, of size m
     */
    void setRightHandSide( const double *b );

    /*
      Initialize the basis matrix to a certain set of basic variables
      Non basic variables are set to their lower bounds, assignment is recomputed
    */
    void initializeBasis( const Set<unsigned> &basicVariables );

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

    // Currently for debug purposes
    const double *getCostFunction();

    void dump()
    {
        printf( "\nDumping A:\n" );
        for ( unsigned i = 0; i < _m; ++i )
        {
            for ( unsigned j = 0; j < _n; ++j )
            {
                printf( "%5.1lf ", _A[j * _m + i] );
            }
            printf( "\n" );
        }

        printf( "\nDumping B:\n" );
        for ( unsigned i = 0; i < _m; ++i )
        {
            for ( unsigned j = 0; j < _m; ++j )
            {
                printf( "%5.1lf ", _B[j * _m + i] );
            }
            printf( "\n" );
        }

        printf( "\nDumping AN:\n" );
        for ( unsigned i = 0; i < _m; ++i )
        {
            for ( unsigned j = 0; j < _n - _m; ++j )
            {
                printf( "%5.1lf ", _AN[j * _m + i] );
            }
            printf( "\n" );
        }
    }

    /* Picks the entering variable. Refreshes the cost function. */
    void pickEnteringVariable();
    unsigned getEnteringVariable() const;

    /* Pick the leaving variable according to the entering variable.
       d is the column vector for the entering variable (length m) */
    void pickLeavingVariable( double *d );
    unsigned getLeavingVariable() const;
    double getChangeRatio() const;

    /* Performs the pivot operation after the entering and leaving
       variables have been selected */
    void performPivot();

    /* Attempt to find a feasible solution. Returns true if found,
       false if problem is infeasible. */
    bool solve();

    /* Calculate the ratio constraint for the entering variable
       imposed by a basic variable.
       Coefficient is the relevant coefficient in the tableau.
       Decrease is true iff the entering variable is decreasing. */
    double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease );

    /* True iff the variable is basic */
    bool isBasic( unsigned variable ) const;

private:
    /* The dimensions of matrix A */
    unsigned _n;
    unsigned _m;

    /* The matrix */
    double *_A;

    /* The basis matrix, which is a subset of A's columns */
    double *_B;

    /* The non-basic matrix, which is a subset of A's columns */
    double *_AN;

    /* A single column matrix from A */
    double *_a;

    /* Used to compute inv(B)*a */
    double *_d;

    /* The right hand side vector of Ax = b */
    double *_b;

    /* The cost function */
    double *_costFunction;

    /* Mapping between basic variables and indices (length m) */
    unsigned *_basicIndexToVariable;

    /* Mapping between non-basic variables and indices (length n - m) */
    unsigned *_nonBasicIndexToVariable;

    /* Mapping from variable to index, either basic or non-basic */
    unsigned *_variableToIndex;

    /* The set of current basic variables */
    Set<unsigned> _basicVariables;

    /* The position of the non basic variables.
       True at upper bound, false at lower bound */
    bool *_nonBasicAtUpper;

    /* Upper and lower bounds for all variables */
    double *_lowerBounds;
    double *_upperBounds;

    /* The current assignment for the basic variables */
    double *_assignment;

    /* Status of the current assignment */
    AssignmentStatus _assignmentStatus;

    /* The current status of the basic variabels */
    unsigned *_basicStatus;

    /* A non-basic variable chosen to become basic in this iteration */
    unsigned _enteringVariable;

    /* A basic variable chosen to become non-basic in this iteration */
    unsigned _leavingVariable;

    /* The amount by which the entering variable should change in this
       pivot step */
    double _changeRatio;

    /* True if the leaving variable increases, false otherwise */
    bool _leavingVariableIncreases;

    /* Extract d for the equation M * d = a. Assume M is upper triangular, and is stored column-wise. */
    // void solveForwardMultiplication( const double *M, double *d, const double *a );

    /* Compute the assignment. Assumes the basis has just been refactored */
    void computeAssignment();

    /* Compute the cost function */
    void computeCostFunction();
    void addRowToCostFunction( unsigned row, double weight );

    /* True if the basic variable is out of bounds */
    bool basicOutOfBounds( unsigned basic ) const;
    bool basicTooHigh( unsigned basic ) const;
    bool basicTooLow( unsigned basic ) const;

    /* True if there exists some out-of-bounds basic */
    bool existsBasicOutOfBounds() const;

    /* Compute the status of the basic variable based on current assignment */
    void computeBasicStatus();
    void computeBasicStatus( unsigned basic );

    /* Compute _d = inv(B)*a */
    void computeD();
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
