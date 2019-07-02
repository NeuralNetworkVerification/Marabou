/*********************                                                        */
/*! \file SparseConstraintMatrixAnalyzer.h
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

#ifndef __SparseConstraintMatrixAnalyzer_h__
#define __SparseConstraintMatrixAnalyzer_h__

#include "List.h"
#include "SparseUnsortedList.h"

class String;

class SparseConstraintMatrixAnalyzer
{
public:
    SparseConstraintMatrixAnalyzer( const double *constraintMatrix, unsigned m, unsigned n );
    ~SparseConstraintMatrixAnalyzer();
    void freeMemoryIfNeeded();

    /*
      Analyze the input matrix in order to find its canonical form
      and rank. The matrix is m by n, and is assumed to be in column-
      major format.
    */
    void analyze();

    unsigned getRank() const;
    List<unsigned> getIndependentColumns() const;
    Set<unsigned> getRedundantRows() const;

    void getCanonicalForm( double *matrix );
    /*
      Debugging: print the matrix and the log message, if logging
      is enabled.
    */
    void dumpMatrix();

private:
    SparseUnsortedList **_A;

    unsigned _m;
    unsigned _n;

    /*
      The i'th row of the (permuted) matrix is stored in memory
      location _rowHeaders[i]. Likewise for columns.
    */
    unsigned *_rowHeaders;
    unsigned *_columnHeaders;

    /*
      Inverse lookup: the row stored in the i'th position in memory is
      the _inverseRowHeaders[i]'th row of the permuted
      matrix. Likewise for columns.
    */
    unsigned *_inverseRowHeaders;
    unsigned *_inverseColumnHeaders;
    double *_work;
    double *_work2;

    unsigned _eliminationStep;
    // bool _logging;
    List<unsigned> _independentColumns;

    /*
      Helper functions for performing Gaussian elimination.
    */
    void gaussianElimination();
    void swapRows( unsigned i, unsigned j );
    void swapColumns( unsigned i, unsigned j );
};

#endif // __SparseConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
