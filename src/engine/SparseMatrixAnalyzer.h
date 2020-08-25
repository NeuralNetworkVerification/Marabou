/*********************                                                        */
/*! \file SparseMatrixAnalyzer.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SparseMatrixAnalyzer_h__
#define __SparseMatrixAnalyzer_h__

#include "List.h"
#include "Set.h"
#include "SparseUnsortedArrays.h"

class String;

class SparseMatrixAnalyzer
{
public:
    SparseMatrixAnalyzer();
    ~SparseMatrixAnalyzer();
    void freeMemoryIfNeeded();

    /*
      Analyze the input matrix in order to find its canonical form
      and rank. The matrix is m by n, and is assumed to be in column-
      major format.
    */
    void analyze( const double *matrix, unsigned m, unsigned n );
    // void analyze( const SparseMatrix *matrix, unsigned m, unsigned n );
    List<unsigned> getIndependentColumns() const;
    Set<unsigned> getRedundantRows() const;

private:
    unsigned _m;
    unsigned _n;
    unsigned _eliminationStep;
    List<unsigned> _independentColumns;

    SparseUnsortedArrays _A;
    SparseUnsortedArrays _At;

    unsigned *_numRowElements;
    unsigned *_numColumnElements;

    unsigned _pivotRow;
    unsigned _pivotColumn;
    double _pivotElement;

    double *_workRow;
    double *_workRow2;

    /*
      The i'th (permuted) column of the matrix is stored in memory
      location _rowHeaders[i]. Likewise for columns.
    */
    unsigned *_rowHeaders;
    unsigned *_columnHeaders;
    unsigned *_rowHeadersInverse;
    unsigned *_columnHeadersInverse;

    /*
      Helper functions for performing Gaussian elimination.
    */
    void gaussianElimination();
    bool choosePivot();
    void permute();
    void eliminate();
};

#endif // __SparseMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
