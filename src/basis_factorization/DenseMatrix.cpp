/*********************                                                        */
/*! \file DenseMatrix.cpp
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

#include "DenseMatrix.h"

DenseMatrix::DenseMatrix( const double *M, unsigned m, unsigned n )
    : _m( m )
    , _n( n )
    , _A( NULL )
{
    allocateMemory();
}

DenseMatrix::DenseMatrix()
    : _m( 0 )
    , _n( 0 )
    , _A( NULL )
{
}

~DenseMatrix::DenseMatrix()
{
    freeMemoryIfNeeded();
}

void DenseMatrix::freeMemoryIfNeeded()
{
    if ( _A )
    {
        delete[] _A;
        _A = NULL;
    }
}

void DenseMatrix::allocateMemory()
{
    _A = new double[_m * _n];
    if ( !_A )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "DenseMatrix::A" );
}

void DenseMatrix::initialize( const double *M, unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    freeMemoryIfNeeded();
    allocateMemory();
    memcpy( _A, M, sizeof(double) * m * n );
}

void DenseMatrix::initializeToEmpty( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    freeMemoryIfNeeded();
    allocateMemory();
    std::fill_n( _A, m * n, 0.0 );
}

double DenseMatrix::get( unsigned row, unsigned column ) const
{
    return _A[_m*row + column];
}

void DenseMatrix::getRow( unsigned row, SparseVector *result ) const
{
    result.clear();

    for ( unsigned i = 0; i < _n; ++i )
    {
        double value = _A[row*_n + i];
        if ( !FloatUtils::isZero( value ) )
            _values[i] = value;
    }
}

void DenseMatrix::getRowDense( unsigned row, double *result ) const
{
    memcpy( result, _A + ( _n * row ), sizeof(double) * _n );
}

void DenseMatrix::getColumn( unsigned column, SparseVector *result ) const
{
    result.clear();

    for ( unsigned i = 0; i < _m; ++i )
    {
        double value = _A[i*_n + column];
        if ( !FloatUtils::isZero( value ) )
            _values[i] = value;
    }
}

void DenseMatrix::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = _A[i*_n + column];
}


void DenseMatrix::addLastRow( double */* row */ )
{
    printf( "Error! DenseMatrix::addLastRow not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::addLastColumn( double */* column */ )
{
    printf( "Error! DenseMatrix::addLastColumn not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::addEmptyColumn();
{
    printf( "Error! DenseMatrix::addEmptyColumn not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::commitChange( unsigned /* row */, unsigned /* column */, double /* newValue */ )
{
    printf( "Error! DenseMatrix::commitChange not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::executeChanges()
{
    printf( "Error! DenseMatrix::executeChanges not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::countElements( unsigned */* numRowElements */, unsigned */* numColumnElements */ )
{
    printf( "Error! DenseMatrix::countElements not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::transposeIntoOther( SparseMatrix */* other */ )
{
    printf( "Error! DenseMatrix::transposeIntoOther not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::dump() const
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        for ( unsigned j = 0; j < _n; ++j )
        {
            printf( "%5.2lf ", _A[i*_n + j] );
        }
        printf( "\n" );
    }
    printf( "\n" );
}

void DenseMatrix::dumpDense() const
{
    dump();
}

void DenseMatrix::storeIntoOther( SparseMatrix */* other */ ) const
{
    printf( "Error! DenseMatrix::storeIntoOther not yet suppoerted!\n" );
    exit( 1 );
}

void DenseMatrix::mergeColumns( unsigned /* x1 */, unsigned /* x2 */ )
{
    printf( "Error! DenseMatrix::mergeColumns not yet suppoerted!\n" );
    exit( 1 );
}

unsigned DenseMatrix::getNnz() const
{
    printf( "Error! DenseMatrix::getNnz not yet suppoerted!\n" );
    exit( 1 );
}

void toDense( double *result ) const
{
    memcpy( result, _A, sizeof(double) * _m * _n );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
