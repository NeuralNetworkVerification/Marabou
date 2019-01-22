/*********************                                                        */
/*! \file GaussianEliminator.h
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

#ifndef __GaussianEliminator_h__
#define __GaussianEliminator_h__

#include "LUFactors.h"
#include "MString.h"

class GaussianEliminator
{
public:
    GaussianEliminator( unsigned m );
    ~GaussianEliminator();

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

#endif // __GaussianEliminator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
