/*********************                                                        */
/*! \file SparseGaussianEliminator.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __SparseGaussianEliminator_h__
#define __SparseGaussianEliminator_h__

#include "MString.h"
#include "SparseColumnsOfBasis.h"
#include "SparseLUFactors.h"
#include "SparseMatrix.h"
#include "Statistics.h"

#define SGAUSSIAN_LOG( x, ... ) LOG( GlobalConfiguration::GAUSSIAN_ELIMINATION_LOGGING, "SparseGaussianEliminator: %s\n", x )

class SparseGaussianEliminator
{
public:
    SparseGaussianEliminator( unsigned m );
    ~SparseGaussianEliminator();

    /*
      The class' main method: perform LU-factorization of a given matrix A,
      provided in row-wise format. Store the results in the provided SparseLUFactors.
    */
    void run( const SparseColumnsOfBasis *A, SparseLUFactors *sparseLUFactors );

    /*
      Have the eliminator start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

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
    double _pivotElement;

    /*
      The output factorization
    */
    SparseLUFactors *_sparseLUFactors;

    /*
      Work memory
    */
    double *_work;
    double *_work2;

    /*
      An object for reporting statistics
    */
    Statistics *_statistics;

    /*
      Information on the number of non-zero elements in
      every row of the current active submatrix of U
    */
    unsigned *_numURowElements;
    unsigned *_numUColumnElements;

    void choosePivot();
    void initializeFactorization( const SparseColumnsOfBasis *A, SparseLUFactors *sparseLUFactors );
    void factorize();
    void permute();
    void eliminate();
};

#endif // __SparseGaussianEliminator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
