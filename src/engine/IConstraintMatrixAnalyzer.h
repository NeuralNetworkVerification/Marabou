/*********************                                                        */
/*! \file IConstraintMatrixAnalyzer.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Shantanu Thakoor, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __IConstraintMatrixAnalyzer_h__
#define __IConstraintMatrixAnalyzer_h__

#include "List.h"

class SparseMatrix;

class IConstraintMatrixAnalyzer
{
public:
    virtual ~IConstraintMatrixAnalyzer() {};

    virtual void analyze( const double *matrix, unsigned m, unsigned n ) = 0;
    virtual void analyze( const SparseMatrix *matrix, unsigned m, unsigned n ) = 0;
    virtual unsigned getRank() const = 0;
    virtual List<unsigned> getIndependentColumns() const = 0;
};

#endif // __IConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
