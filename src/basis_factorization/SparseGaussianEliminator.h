/*********************                                                        */
/*! \file SparseGaussianEliminator.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseGaussianEliminator_h__
#define __SparseGaussianEliminator_h__

#include "LUFactors.h"
#include "MString.h"

class SparseGaussianEliminator
{
public:
    SparseGaussianEliminator( unsigned m );
    ~SparseGaussianEliminator();

    /*
      The class' main method: perform LU-factorization of a given matrix A,
      provided in row-wise format. Store the results in the provided LUFactors.
    */
    void run( const double *A, LUFactors *luFactors );

private:
    /*
      The dimension of the (square) matrix being factorized
    */
    unsigned _m;

    /*
      Information on the current elimination step
    */
    unsigned _uPivotRow;
    unsigned _uPivotColumn;
    unsigned _vPivotRow;
    unsigned _vPivotColumn;
    unsigned _eliminationStep;

    /*
      The output factorization
    */
    LUFactors *_luFactors;

    /*
      Information on the number of non-zero elements in
      every row of the current active submatrix of U
    */
    unsigned *_numURowElements;
    unsigned *_numUColumnElements;

    void choosePivot();
    void initializeFactorization( const double *A, LUFactors *luFactors );
    void permute();
    void eliminate();

    void log( const String &message );
};

#endif // __SparseGaussianEliminator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
