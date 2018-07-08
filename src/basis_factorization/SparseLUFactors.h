/*********************                                                        */
/*! \file SparseLUFactors.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseLUFactors_h__
#define __SparseLUFactors_h__

#include "PermutationMatrix.h"
#include "SparseMatrix.h"

/*
  This class provides supprot for an LU-factorization of a given matrix.

  The factorization has the following form:

     A = F * V = P * L * U * Q

     F = P * L * P'
     V = P * U * Q

  Where:
    - A is the matrix being factorized
    - L is lower triangular with all diagonal entries equal to 1
    - U is upper triangular
    - P and Q are permutation matrices

  Matrices F, V, P and Q are stored sparsely. Matrices
  U and L are not stored at all, as they are implied by the rest.

  The class also provides functionality for basic computations involving
  the factorization.
*/
class SparseLUFactors
{
public:
    SparseLUFactors( unsigned m );
    ~SparseLUFactors();

    /*
      The dimension of all matrices involved
    */
    unsigned _m;

    /*
      The various factorization components as described above
    */
    SparseMatrix *_F;
    SparseMatrix *_V;
    PermutationMatrix _P;
    PermutationMatrix _Q;

    /*
      Basic computations (BTRAN, FTRAN) involving the factorization

      forwardTransformation: find x such that Ax ( = FVx ) = y
      backwardTransformation: find x such that xA ( = xFV ) = y

      fForwardTransformation:  find x such that Fx = y
      fBackwardTransformation: find x such that xF = y
      vForwardTransformation:  find x such that Vx = y
      vBackwardTransformation: find x such that xV = y

      In all functions, x contains y on entry, and contains the solution
      on exit.
    */
    void forwardTransformation( const double *y, double *x ) const;
    void backwardTransformation( const double *y, double *x ) const;

    void fForwardTransformation( const double *y, double *x ) const;
    void fBackwardTransformation( const double *y, double *x ) const;
    void vForwardTransformation( const double *y, double *x ) const;
    void vBackwardTransformation( const double *y, double *x ) const;

    /*
      Compute the inverse of the factorized basis
    */
    void invertBasis( double *result );

    /*
      Work memory
    */
    double *_z;
    double *_workMatrix;

    /*
      Clone this SparseLUFactors object into another object
    */
    void storeToOther( SparseLUFactors *other ) const;

    /*
      For debugging purposes
    */
    void dump() const;
};

#endif // __SparseLUFactors_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
