/********************                                                        */
/*! \file PermutationMatrix.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "PermutationMatrix.h"
#include <cstdlib>

PermutationMatrix::PermutationMatrix( unsigned m )
    : _m( m )
    , _ordering( NULL )
{
    _ordering = new unsigned[m];
    if ( !_ordering )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "PermutationMatrix::ordering" );

    resetToIdentity();
}

PermutationMatrix::~PermutationMatrix()
{
    if ( _ordering )
    {
        delete[] _ordering;
        _ordering = NULL;
    }
}

void PermutationMatrix::resetToIdentity()
{
    for ( unsigned i = 0; i < _m; ++i )
        _ordering[i] = i;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
