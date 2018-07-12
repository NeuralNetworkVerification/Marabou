/*********************                                                        */
/*! \file ConstraintMatrixAnalyzer.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ConstraintMatrixAnalyzer_h__
#define __ConstraintMatrixAnalyzer_h__

#include "IConstraintMatrixAnalyzer.h"
#include "List.h"
#include "SparseMatrix.h"

class String;

class ConstraintMatrixAnalyzer : public IConstraintMatrixAnalyzer
{
public:
    ConstraintMatrixAnalyzer();
    ~ConstraintMatrixAnalyzer();
    void freeMemoryIfNeeded();

    /*
      Analyze the input matrix in order to find its canonical form
      and rank. The matrix is m by n, and is assumed to be in column-
      major format.
    */
    void analyze( const double *matrix, unsigned m, unsigned n );
    void analyze( const SparseMatrix *matrix, unsigned m, unsigned n );
    void getCanonicalForm( double *matrix );
    unsigned getRank() const;
    List<unsigned> getIndependentColumns() const;

private:
    double *_matrix;
    double *_work;
    unsigned _m;
    unsigned _n;
    unsigned _eliminationStep;
    bool _logging;
    List<unsigned> _independentColumns;

    /*
      The i'th (permuted) column of the matrix is stored in memory
      location _rowHeaders[i]. Likewise for columns.
    */
    unsigned *_rowHeaders;
    unsigned *_columnHeaders;

    /*
      Helper functions for performing Gaussian elimination.
    */
    void gaussianElimination();
    void swapRows( unsigned i, unsigned j );
    void swapColumns( unsigned i, unsigned j );

    /*
      Debugging: print the matrix and the log message, if logging
      is enabled.
    */
    void dumpMatrix( const String &message );
};

#endif // __ConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
