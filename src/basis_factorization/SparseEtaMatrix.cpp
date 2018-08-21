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
    , _sparseColumn( column, m )
{
    _diagonalElement = column[index];
}

SparseEtaMatrix::SparseEtaMatrix( unsigned m, unsigned index )
    : _m( m )
    , _columnIndex( index )
    , _sparseColumn( m )
{
    _sparseColumn.commitChange( index, 1 );
    _sparseColumn.executeChanges();
    _diagonalElement = 1;
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
    for ( unsigned i = 0; i < _sparseColumn.getNnz(); ++i )
        printf( "\t\tEntry %u: %lf", _sparseColumn.getIndexOfEntry( i ), _sparseColumn.getValueOfEntry( i ) );
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

void SparseEtaMatrix::resetToIdentity()
{
    _sparseColumn.clear();
    _sparseColumn.commitChange( _columnIndex, 1 );
    _sparseColumn.executeChanges();
    _diagonalElement = 1;
}

void SparseEtaMatrix::commitChange( unsigned index, double newValue )
{
    _sparseColumn.commitChange( index, newValue );
}

void SparseEtaMatrix::executeChanges()
{
    _sparseColumn.executeChanges();
    _diagonalElement = _sparseColumn.get( _columnIndex );
}

void SparseEtaMatrix::dumpDenseTransposed() const
{
    printf( "Dumping transposed eta matrix:\n" );
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
                printf( "%5.2lf ", _sparseColumn.get( j ) );
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

    for ( unsigned i = 0; i < _m; ++i )
        A[i*_m + _columnIndex] = _sparseColumn.get( i );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
