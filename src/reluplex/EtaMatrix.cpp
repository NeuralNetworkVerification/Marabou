/*********************                                                        */
/*! \file EtaMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "EtaMatrix.h"
#include "ReluplexError.h"

#include <cstdlib>
#include <cstring>

EtaMatrix::EtaMatrix( unsigned m, unsigned index, double *column )
    : _m( m )
    , _columnIndex( index )
    , _column( NULL )
{
    _column = new double[_m];
    if ( !_column )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "EtaMatrix::column" );

    memcpy( _column, column, sizeof(double) * _m );
}

EtaMatrix::~EtaMatrix()
{
    if ( _column )
    {
        delete[] _column;
        _column = NULL;
    }
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
