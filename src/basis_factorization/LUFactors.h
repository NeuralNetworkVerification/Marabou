/*********************                                                        */
/*! \file LUFactors.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __LUFactors_h__
#define __LUFactors_h__

#include "PermutationMatrix.h"

class LUFactors
{

/***********************************************************************
*  The structure LUF describes sparse LU-factorization.
*
*  The LU-factorization has the following format:
*
*     A = F * V = P * L * U * Q,                                     (1)
*
*     F = P * L * P',                                                (2)
*
*     V = P * U * Q,                                                 (3)
*
*  where A is a given (unsymmetric) square matrix, F and V are matrix
*  factors actually computed, L is a lower triangular matrix with unity
*  diagonal, U is an upper triangular matrix, P and Q are permutation
*  matrices, P' is a matrix transposed to P. All the matrices have the
*  same order n.
*
*  Matrices F and V are stored in both row- and column-wise sparse
*  formats in the associated sparse vector area (SVA). Unity diagonal
*  elements of matrix F are not stored. Pivot elements of matrix V
*  (which correspond to diagonal elements of matrix U) are stored in
*  a separate ordinary array.
*
*  Permutation matrices P and Q are stored in ordinary arrays in both
*  row- and column-like formats.
*
*  Matrices L and U are completely defined by matrices F, V, P, and Q,
*  and therefore not stored explicitly. */

public:
    LUFactors( unsigned m );
    ~LUFactors();

    unsigned _m;

    double *_F;
    double *_V;
    PermutationMatrix _P;
    PermutationMatrix _Q;

    void dump() const;
};

#endif // __LUFactors_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
