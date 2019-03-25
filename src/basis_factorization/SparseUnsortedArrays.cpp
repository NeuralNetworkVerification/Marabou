/*********************                                                        */
/*! \file SparseUnsortedArrays.cpp
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

#include "BasisFactorizationError.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "SparseUnsortedArrays.h"
#include <cstring>

SparseUnsortedArrays::SparseUnsortedArrays()
    : _rows( NULL )
    , _m( 0 )
    , _n( 0 )
{
}

SparseUnsortedArrays::~SparseUnsortedArrays()
{
    freeMemoryIfNeeded();
}

void SparseUnsortedArrays::freeMemoryIfNeeded()
{
    if ( _rows )
    {
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( _rows[i] )
            {
                delete _rows[i];
                _rows[i] = NULL;
            }
        }

        delete[] _rows;
        _rows = NULL;
    }
}

void SparseUnsortedArrays::initialize( const double *M, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedArray *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedArray;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows[i]" );

        _rows[i]->initialize( M + ( i * n ), n );
    }
}

void SparseUnsortedArrays::initialize( const SparseUnsortedArray **V, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedArray *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedArray;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows[i]" );

        V[i]->storeIntoOther( _rows[i] );
    }
}

void SparseUnsortedArrays::initializeToEmpty( unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedArray *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedArray( _n );
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::rows[i]" );
    }
}

void SparseUnsortedArrays::updateSingleRow( unsigned row, const double *dense )
{
    _rows[row]->initialize( dense, _n );
}

double SparseUnsortedArrays::get( unsigned row, unsigned column ) const
{
    return _rows[row]->get( column );
}

void SparseUnsortedArrays::set( unsigned row, unsigned column, double value )
{
    _rows[row]->set( column, value );
}

const SparseUnsortedArray *SparseUnsortedArrays::getRow( unsigned row ) const
{
    return _rows[row];
}

SparseUnsortedArray *SparseUnsortedArrays::getRow( unsigned row )
{
    return _rows[row];
}

void SparseUnsortedArrays::getRowDense( unsigned row, double *result ) const
{
    _rows[row]->toDense( result );
}

void SparseUnsortedArrays::getColumn( unsigned column, SparseUnsortedArray *result ) const
{
    result->clear();

    for ( unsigned i = 0; i < _m; ++i )
        result->set( i, _rows[i]->get( column ) );
}

void SparseUnsortedArrays::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void SparseUnsortedArrays::addLastRow( const double *row )
{
    SparseUnsortedArray **newRows = new SparseUnsortedArray *[_m + 1];
    if ( !newRows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::newRows" );

    memcpy( newRows, _rows, sizeof(SparseUnsortedArray *) * _m );

    newRows[_m] = new SparseUnsortedArray;
    if ( !newRows[_m] )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "ArrayOfCSRUnsortedLists::newRows[_m]" );

    newRows[_m]->initialize( row, _n );

    delete[] _rows;
    _rows = newRows;

    ++_m;
}

void SparseUnsortedArrays::addLastColumn( const double *column )
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->addLastEntry( column[i] );

    ++_n;
}

void SparseUnsortedArrays::addEmptyColumn()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->incrementSize();

    ++_n;
}

void SparseUnsortedArrays::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    std::fill_n( numRowElements, _n, 0 );
    std::fill_n( numColumnElements, _n, 0 );

    SparseUnsortedArray::Entry entry;

    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _rows[i]->getNnz(); ++j )
        {
            entry = _rows[i]->getByArrayIndex( j );

            if ( !FloatUtils::isZero( entry._value ) )
            {
                ++numRowElements[i];
                ++numColumnElements[entry._index];
            }
        }
    }
}

void SparseUnsortedArrays::transposeIntoOther( SparseUnsortedArrays *other )
{
    other->initializeToEmpty( _n, _m );

    SparseUnsortedArray::Entry entry;

    for ( unsigned row = 0; row < _m; ++row )
    {
        for ( unsigned i = 0; i < _rows[row]->getNnz(); ++i )
        {
            entry = _rows[row]->getByArrayIndex( i );
            other->_rows[entry._index]->append( row, entry._value );
        }
    }
}

void SparseUnsortedArrays::dump() const
{
    dumpDense();
}

void SparseUnsortedArrays::dumpDense() const
{
    printf( "\n" );
    for ( unsigned row = 0; row < _m; ++row )
    {
        printf( "\t" );
        _rows[row]->dumpDense();
        printf( "\n" );
    }
    printf( "\n\n" );
}

void SparseUnsortedArrays::storeIntoOther( SparseUnsortedArrays *other ) const
{
    other->freeMemoryIfNeeded();

    other->_m = _m;
    other->_n = _n;

    other->_rows = new SparseUnsortedArray *[_m];
    if ( !other->_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::otherRows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        other->_rows[i] = new SparseUnsortedArray;
        if ( !other->_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedArrays::otherRows[i]" );

        _rows[i]->storeIntoOther( other->_rows[i] );
    }
}

unsigned SparseUnsortedArrays::getNnz() const
{
    unsigned result = 0;
    for ( unsigned i = 0; i < _m; ++i )
        result += _rows[i]->getNnz();

    return result;
}

void SparseUnsortedArrays::toDense( double *result ) const
{
    std::fill_n( result, _n * _m, 0 );

    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->toDense( result + ( i * _n ) );
}

void SparseUnsortedArrays::clear()
{
    printf( "Clear called!\n" );

    for ( unsigned i = 0; i < _m; ++i )
        clear( i );
}

void SparseUnsortedArrays::clear( unsigned row )
{
    _rows[row]->clear();
}

void SparseUnsortedArrays::append( unsigned row, unsigned column, double value )
{
    _rows[row]->append( column, value );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
