/*********************                                                        */
/*! \file SparseColumnsOfBasis.cpp
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
#include "SparseColumnsOfBasis.h"

SparseColumnsOfBasis::SparseColumnsOfBasis( unsigned m )
    : _columns( NULL )
    , _m( m )
{
    _columns = new const SparseVector *[m];
    if ( !_columns )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseColumnsOfBasis::columns" );
}

SparseColumnsOfBasis::~SparseColumnsOfBasis()
{
    if ( _columns )
    {
        delete[] _columns;
        _columns = NULL;
    }
}

void SparseColumnsOfBasis::dump() const
{
    for ( unsigned i = 0; i < _m; ++i )
        _columns[i]->dumpDense();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
