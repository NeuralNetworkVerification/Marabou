/*********************                                                        */
/*! \file GaussianEliminator.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __GaussianEliminator_h__
#define __GaussianEliminator_h__

#include "LPElement.h"
#include "List.h"

class GaussianEliminator
{
public:
    enum FactorizationStrategy {
        PARTIAL_PIVOTING = 0,
        MARKOWITZ,
    };

    GaussianEliminator( const double *A, unsigned m );
    ~GaussianEliminator();

    /*
      The classes main method: factorize matrix A, given in row-major format,
      into a matrix U and a sequence of L and P matrices, such that:

      - U is upper triangular with its diagonal set to 1s
      - The Ls are lower triangular
      - The Ps are permutation matrices
      - The rowHeaders array indicates the orders of the rows of U, where the i'th
        row is stored in the rowHeaders[i] location in memory
    */
    void factorize( List<LPElement *> *lp,
                    double *U,
                    unsigned *rowHeaders,
                    FactorizationStrategy factorizationStrategy = PARTIAL_PIVOTING );

private:
    /*
      The (square) matrix being factorized and its dimension
    */
    const double *_A;
    unsigned _m;

    /*
      Given a column with element in indices [columnIndex, .., _m], choose the next pivot according to
      the factorization strategy.
    */
    unsigned choosePivotElement( unsigned columnIndex, FactorizationStrategy factorizationStrategy );

    /*
      Work memory
    */
    double *_pivotColumn;
    double *_LCol;
};

#endif // __GaussianEliminator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
