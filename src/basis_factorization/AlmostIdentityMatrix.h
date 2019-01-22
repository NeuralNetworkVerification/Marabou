/*********************                                                        */
/*! \file AlmostIdentityMatrix.h
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

#ifndef __AlmostIdentityMatrix_h__
#define __AlmostIdentityMatrix_h__

#include "List.h"

class AlmostIdentityMatrix
{
public:
    unsigned _row;
    unsigned _column;
    double _value;

    AlmostIdentityMatrix()
    {
    }

    AlmostIdentityMatrix( const AlmostIdentityMatrix &other )
        : _row( other._row )
        , _column( other._column )
        , _value( other._value )
    {
    }

    AlmostIdentityMatrix &operator=( const AlmostIdentityMatrix &other )
    {
        _row = other._row;
        _column = other._column;
        _value = other._value;

        return *this;
    }
};

#endif // __AlmostIdentityMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
