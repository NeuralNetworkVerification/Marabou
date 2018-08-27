/*********************                                                        */
/*! \file SparseColumnsOfBasis.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseColumnsOfBasis_h__
#define __SparseColumnsOfBasis_h__

#include "SparseUnsortedList.h"

class SparseColumnsOfBasis
{
public:
    SparseColumnsOfBasis( unsigned m );
    ~SparseColumnsOfBasis();

    const SparseUnsortedList **_columns;

    /*
      For debugging purposes
    */
    void dump() const;
    unsigned _m;
};

#endif // __SparseColumnsOfBasis_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
