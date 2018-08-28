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
#include "Debug.h"
#include "FloatUtils.h"
#include "SparseUnsortedLists.h"
#include <cstring>

SparseUnsortedLists::SparseUnsortedLists()
    : _rows( NULL )
    , _m( 0 )
    , _n( 0 )
{
}

SparseUnsortedLists::~SparseUnsortedLists()
{
    freeMemoryIfNeeded();
}

void SparseUnsortedLists::freeMemoryIfNeeded()
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

void SparseUnsortedLists::initialize( const double *M, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedList *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedList;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows[i]" );

        _rows[i]->initialize( M + ( i * n ), n );
    }
}

void SparseUnsortedLists::initialize( const SparseUnsortedList **V, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedList *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedList;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows[i]" );

        V[i]->storeIntoOther( _rows[i] );
    }
}

void SparseUnsortedLists::initializeToEmpty( unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedList *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedList( _n );
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::rows[i]" );
    }
}

void SparseUnsortedLists::updateSingleRow( unsigned row, const double *dense )
{
    _rows[row]->initialize( dense, _n );
}

double SparseUnsortedLists::get( unsigned row, unsigned column ) const
{
    return _rows[row]->get( column );
}

void SparseUnsortedLists::set( unsigned row, unsigned column, double value )
{
    _rows[row]->set( column, value );
}

const SparseUnsortedList *SparseUnsortedLists::getRow( unsigned row ) const
{
    return _rows[row];
}

SparseUnsortedList *SparseUnsortedLists::getRow( unsigned row )
{
    return _rows[row];
}

void SparseUnsortedLists::getRowDense( unsigned row, double *result ) const
{
    _rows[row]->toDense( result );
}

void SparseUnsortedLists::getColumn( unsigned column, SparseUnsortedList *result ) const
{
    result->clear();

    for ( unsigned i = 0; i < _m; ++i )
        result->set( i, _rows[i]->get( column ) );
}

void SparseUnsortedLists::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void SparseUnsortedLists::addLastRow( const double *row )
{
    SparseUnsortedList **newRows = new SparseUnsortedList *[_m + 1];
    if ( !newRows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::newRows" );

    memcpy( newRows, _rows, sizeof(SparseUnsortedList *) * _m );

    newRows[_m] = new SparseUnsortedList;
    if ( !newRows[_m] )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "ArrayOfCSRUnsortedLists::newRows[_m]" );

    newRows[_m]->initialize( row, _n );

    delete[] _rows;
    _rows = newRows;

    ++_m;
}

void SparseUnsortedLists::addLastColumn( const double *column )
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->addLastEntry( column[i] );

    ++_n;
}

void SparseUnsortedLists::addEmptyColumn()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->incrementSize();

    ++_n;
}

void SparseUnsortedLists::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    std::fill_n( numColumnElements, _n, 0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        // Rows
        numRowElements[i] = _rows[i]->getNnz();

        // Columns
        for ( const auto element : (*_rows[i]) )
            ++numColumnElements[element._index];
    }
}

void SparseUnsortedLists::transposeIntoOther( SparseUnsortedLists *other )
{
    other->initializeToEmpty( _n, _m );

    for ( unsigned row = 0; row < _m; ++row )
    {
        for ( const auto &element : (*_rows[row]) )
        {
            other->_rows[element._index]->append( row, element._value );
        }
    }
}

void SparseUnsortedLists::dump() const
{
    dumpDense();
}

void SparseUnsortedLists::dumpDense() const
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

void SparseUnsortedLists::storeIntoOther( SparseUnsortedLists *other ) const
{
    other->freeMemoryIfNeeded();

    other->_m = _m;
    other->_n = _n;

    other->_rows = new SparseUnsortedList *[_m];
    if ( !other->_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::otherRows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        other->_rows[i] = new SparseUnsortedList;
        if ( !other->_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedLists::otherRows[i]" );

        _rows[i]->storeIntoOther( other->_rows[i] );
    }
}

unsigned SparseUnsortedLists::getNnz() const
{
    unsigned result = 0;
    for ( unsigned i = 0; i < _m; ++i )
        result += _rows[i]->getNnz();

    return result;
}

void SparseUnsortedLists::toDense( double *result ) const
{
    std::fill_n( result, _n * _m, 0 );

    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->toDense( result + ( i * _n ) );
}

void SparseUnsortedLists::clear()
{
    for ( unsigned i = 0; i < _m; ++i )
        clear( i );
}

void SparseUnsortedLists::clear( unsigned row )
{
    _rows[row]->clear();
}

void SparseUnsortedLists::append( unsigned row, unsigned column, double value )
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
