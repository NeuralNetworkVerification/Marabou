/********************                                                        */
/*! \file SparseEtaMatrix.cpp
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
#include "SparseEtaMatrix.h"
#include "FloatUtils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

SparseEtaMatrix::SparseEtaMatrix( unsigned m, unsigned index, const double *column )
    : _m( m )
    , _columnIndex( index )
{
    _diagonalElement = column[index];
    for ( unsigned i = 0; i < m; ++i )
    {
        if ( !FloatUtils::isZero( column[i] ) )
            addEntry( i, column[i] );
    }
}

SparseEtaMatrix::SparseEtaMatrix( unsigned m, unsigned index )
    : _m( m )
    , _columnIndex( index )
    , _diagonalElement( 0 )
{
}

SparseEtaMatrix::SparseEtaMatrix( const SparseEtaMatrix &other )
    : _m( other._m )
    , _columnIndex( other._columnIndex )
    , _sparseColumn( other._sparseColumn )
    , _diagonalElement( other._diagonalElement )
{
}

SparseEtaMatrix &SparseEtaMatrix::operator=( const SparseEtaMatrix &other )
{
    _m = other._m;
    _columnIndex = other._columnIndex;
    _sparseColumn = other._sparseColumn;
    _diagonalElement = other._diagonalElement;

    return *this;
}

SparseEtaMatrix::~SparseEtaMatrix()
{
}

void SparseEtaMatrix::dump() const
{
    printf( "Dumping eta matrix\n" );
    printf( "\tm = %u. column index = %u\n", _m, _columnIndex );
    printf( "\tcolumn: " );
    for ( const auto &entry : _sparseColumn )
        printf( "\t\tEntry %u: %lf", entry._index, entry._value );
    printf( "\n" );
}

bool SparseEtaMatrix::operator==( const SparseEtaMatrix &other ) const
{
    if ( _m != other._m )
        return false;

    if ( _columnIndex != other._columnIndex )
        return false;

    printf( "Error! SparseEtaMatrix::operator== not yet supported!\n" );
    exit( 1 );

    return true;
}

void SparseEtaMatrix::addEntry( unsigned index, double value )
{
    _sparseColumn.append( Entry( index, value ) );
    if ( index == _columnIndex )
        _diagonalElement = value;
}

void SparseEtaMatrix::dumpDenseTransposed() const
{
    printf( "Dumping transposed eta matrix:\n" );

    double *dense = new double[_m];
    std::fill_n( dense, _m, 0 );
    for ( const auto &entry : _sparseColumn )
        dense[entry._index] = entry._value;

    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        if ( i != _columnIndex )
        {
            // Print identity row
            for ( unsigned j = 0; j < _m; ++j )
            {
                printf( "%5u ", j == i ? 1 : 0 );
            }
        }
        else
        {
            for ( unsigned j = 0; j < _m; ++j )
            {
                printf( "%5.2lf ", dense[j] );
            }
        }
        printf( "\n" );
    }
    printf( "\n" );
}

void SparseEtaMatrix::toMatrix( double *A ) const
{
    std::fill_n( A, _m * _m, 0.0 );

	for ( unsigned i = 0; i < _m; ++i )
		A[i * _m + i] = 1;

    for ( const auto &entry : _sparseColumn )
        A[entry._index * _m + _columnIndex] = entry._value;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
