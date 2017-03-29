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
      Initialize the basis matrix to a certain set of basic variables
      Non basic variables are set to 0, assignment is recomputed
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
      Perform a backward transformation, i.e. find d such that d = inv(B) * a,
      where a is the column of the matrix A that corresponds to the entering variable.

      The solution is found by solving Bd = a.

      Bd = (B_0 * E_1 * E_2 ... * E_n) d
         = B_0 ( E_1 ( ... ( E_n d ) ) ) = a
                           -- u_n --
                     ------- u_1 -------
               --------- u_0 -----------


      And the equation is solved iteratively:
      B_0     * u_0   =   a   --> obtain u_0
      E_1     * u_1   =  u_0  --> obtain u_1
      ...
      E_n     * d     =  u_n  --> obtain d


      result needs to be of size m.
    */
    void backwardTransformation( unsigned enteringVariable, double *result );

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
    }

private:
    /* The dimensions of matrix A */
    unsigned _n;
    unsigned _m;

    /* The matrix */
    double *_A;

    /* The basis matrix, which is a subset of A's columns */
    double *_B;

    /* A single column matrix from A */
    double *_a;

    /* The right hand side vector of Ax = b */
    double *_b;

    /* Mapping between basic variables and indices (length m) */
    unsigned *_basicIndexToVariable;

    /* Mapping between non-basic variables and indices (length m) */
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

    /* Extract d for the equation M * d = a. Assume M is upper triangular, and is stored column-wise. */
    void solveForwardMultiplication( const double *M, double *d, const double *a );
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
