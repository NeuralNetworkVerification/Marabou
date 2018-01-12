/*********************                                                        */
/*! \file AlmostIdentityMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __AlmostIdentityMatrix_h__
#define __AlmostIdentityMatrix_h__

#include "List.h"

class AlmostIdentityMatrix
{
public:
    AlmostIdentityMatrix()
        : _identity( true )
    {
    }

    /*
      True iff this is the identity matrix.
    */
    bool _identity;

    unsigned _row;
    unsigned _column;
    double _value;
};

#endif // __AlmostIdentityMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
