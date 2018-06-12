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
#include "Debug.h"
#include "PermutationMatrix.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

PermutationMatrix::PermutationMatrix( unsigned m )
    : _rowOrdering( NULL )
    , _columnOrdering( NULL )
    , _m( m )
{
    _rowOrdering = new unsigned[m];
    if ( !_rowOrdering )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "PermutationMatrix::rowOrdering" );

    _columnOrdering = new unsigned[m];
    if ( !_columnOrdering )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "PermutationMatrix::columnOrdering" );

    resetToIdentity();
}

PermutationMatrix::~PermutationMatrix()
{
    if ( _rowOrdering )
    {
        delete[] _rowOrdering;
        _rowOrdering = NULL;
    }

    if ( _columnOrdering )
    {
        delete[] _columnOrdering;
        _columnOrdering = NULL;
    }
}

void PermutationMatrix::resetToIdentity()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        _rowOrdering[i] = i;
        _columnOrdering[i] = i;
    }
}

PermutationMatrix &PermutationMatrix::operator=( const PermutationMatrix &other )
{
    if ( _m != other._m )
    {
        _m = other._m;

        if ( _rowOrdering )
            delete[] _rowOrdering;
        _rowOrdering = new unsigned[_m];

        if ( _columnOrdering )
            delete[] _columnOrdering;
        _columnOrdering = new unsigned[_m];
    }

    memcpy( _rowOrdering, other._rowOrdering, sizeof(unsigned) * _m );
    memcpy( _columnOrdering, other._columnOrdering, sizeof(unsigned) * _m );

    return *this;
}

PermutationMatrix *PermutationMatrix::invert() const
{
    PermutationMatrix *inverse = new PermutationMatrix( _m );

    for ( unsigned i = 0; i < _m; ++i )
    {
        inverse->_rowOrdering[_rowOrdering[i]] = i;
        inverse->_columnOrdering[_columnOrdering[i]] = i;
    }

    return inverse;
}

void PermutationMatrix::invert( PermutationMatrix &inv ) const
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        inv._rowOrdering[_rowOrdering[i]] = i;
        inv._columnOrdering[_columnOrdering[i]] = i;
    }
}

unsigned PermutationMatrix::findIndexOfRow( unsigned row ) const
{
    ASSERT( row < _m );

    for ( unsigned i = 0; i < _m; ++i )
        if ( _rowOrdering[i] == row )
            return i;

    throw BasisFactorizationError( BasisFactorizationError::CORRUPT_PERMUATION_MATRIX );
}

unsigned PermutationMatrix::getM() const
{
    return _m;
}

void PermutationMatrix::dump() const
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            if ( _rowOrdering[i] == j )
                printf( "1 " );
            else
                printf( "0 " );
        }
        printf( "\n" );
    }
    printf( "\n" );
}

bool PermutationMatrix::isIdentity() const
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( _rowOrdering[i] != i )
            return false;
    }

    return true;
}

void PermutationMatrix::swapRows( unsigned a, unsigned b )
{
    if ( a == b )
        return;

    unsigned tempA = _rowOrdering[a];
    unsigned tempB = _rowOrdering[b];

    _rowOrdering[a] = tempB;
    _rowOrdering[b] = tempA;

    _columnOrdering[tempB] = a;
    _columnOrdering[tempA] = b;
}

void PermutationMatrix::swapColumns( unsigned a, unsigned b )
{
    if ( a == b )
        return;

    unsigned tempA = _columnOrdering[a];
    unsigned tempB = _columnOrdering[b];

    _columnOrdering[a] = tempB;
    _columnOrdering[b] = tempA;

    _rowOrdering[tempB] = a;
    _rowOrdering[tempA] = b;
}

void PermutationMatrix::storeToOther( PermutationMatrix *other ) const
{
    ASSERT( _m == other->_m );

    memcpy( other->_rowOrdering, _rowOrdering, sizeof(unsigned) * _m );
    memcpy( other->_columnOrdering, _columnOrdering, sizeof(unsigned) * _m );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
