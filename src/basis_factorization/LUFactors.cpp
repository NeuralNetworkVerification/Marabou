/*********************                                                        */
/*! \file LUFactors.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "LUFactors.h"
#include "MString.h"

LUFactors::LUFactors( unsigned m )
    : _m( m )
    , _P( m )
    , _Q( m )
{
    _F = new double[m*m];
    if ( !_F )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::F" );

    _V = new double[m*m];
    if ( !_V )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::V" );
}

LUFactors::~LUFactors()
{
    if ( _F )
    {
        delete[] _F;
        _F = NULL;
    }

    if ( _V )
    {
        delete[] _V;
        _V = NULL;
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
