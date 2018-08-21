/*********************                                                        */
/*! \file SparseEtaMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseEtaMatrix_h__
#define __SparseEtaMatrix_h__

#include "SparseVector.h"

class SparseEtaMatrix
{
public:
    SparseEtaMatrix( unsigned m, unsigned index, const double *column );

    /*
      Initializees the matrix to the identity matrix
    */
    SparseEtaMatrix( unsigned m, unsigned index );

    SparseEtaMatrix( const SparseEtaMatrix &other );
    SparseEtaMatrix &operator=( const SparseEtaMatrix &other );

    ~SparseEtaMatrix();
    void dump() const;
    void dumpDenseTransposed() const;
	void toMatrix( double *A ) const;

    void resetToIdentity();

    bool operator==( const SparseEtaMatrix &other ) const;

    void commitChange( unsigned index, double newValue );
    void executeChanges();

    unsigned _m;
    unsigned _columnIndex;
    SparseVector _sparseColumn;
    double _diagonalElement;
};

#endif // __SparseEtaMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
