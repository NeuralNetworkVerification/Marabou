/*********************                                                        */
/*! \file ConstraintMatrixAnalyzer.h
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

#ifndef __ConstraintMatrixAnalyzer_h__
#define __ConstraintMatrixAnalyzer_h__

#include "IConstraintMatrixAnalyzer.h"
#include "List.h"
#include "Set.h"
#include "SparseUnsortedArrays.h"

class String;

class ConstraintMatrixAnalyzer : public IConstraintMatrixAnalyzer
{
public:
    ConstraintMatrixAnalyzer();
    ~ConstraintMatrixAnalyzer();

    /*
      Analyze the input matrix in order to find its sets of
      (in)dependent columns and rows
    */
    void analyze( const double *matrix, unsigned m, unsigned n );
    void analyze( const SparseUnsortedList **matrix, unsigned m, unsigned n );
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
      The i'th (permuted) row of the matrix is stored in memory
      location _rowHeaders[i]. Likewise for columns.
    */
    unsigned *_rowHeaders;
    unsigned *_columnHeaders;
    unsigned *_rowHeadersInverse;
    unsigned *_columnHeadersInverse;

    /*
      Memory management
    */
    void freeMemoryIfNeeded();
    void allocateMemory();

    /*
      Helper functions for performing Gaussian elimination.
    */
    void gaussianElimination();
    bool choosePivot();
    void permute();
    void eliminate();
};

#endif // __ConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
