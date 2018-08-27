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
#include "SparseUnsortedVectors.h"
#include "SparseVectors.h"

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

  On some cases, we may wish to use separate Ps for the F and V computation.
  While the same matrix is used for both by default, the class also supports
  setting a different P for the L computation. In this case, the main equation
  becomes A = FV = ( P1 L P1') * ( P U Q ).

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
    SparseUnsortedVectors *_F;
    SparseUnsortedVectors *_V;
    PermutationMatrix _P;
    PermutationMatrix _Q;

    /*
      A separate matrix, PforF, that is used for computing F = PLP',
      if it is used (otherwise the default P is used). The flag indicates
      whether this matrix is currently in use or not.
    */
    bool _usePForF;
    PermutationMatrix _PForF;

    /*
      The transposed matrics F' and V' are also stored. This is because
      sometimes we need to retrieve columns and sometimes we needs rows,
      and these operations may be cheaper on the transposed matrix
    */
    SparseUnsortedVectors *_Ft;
    SparseUnsortedVectors *_Vt;

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
