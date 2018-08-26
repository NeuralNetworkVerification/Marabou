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
#include "SparseUnsortedVectors.h"
#include <cstring>

SparseUnsortedVectors::SparseUnsortedVectors()
    : _rows( NULL )
    , _m( 0 )
    , _n( 0 )
{
}

SparseUnsortedVectors::~SparseUnsortedVectors()
{
    freeMemoryIfNeeded();
}

void SparseUnsortedVectors::freeMemoryIfNeeded()
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

void SparseUnsortedVectors::initialize( const double *M, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVector;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows[i]" );

        _rows[i]->initialize( M + ( i*_n ), _n );
    }
}

void SparseUnsortedVectors::initialize( const SparseVector **V, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVector;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows[i]" );

        for ( unsigned j = 0; j < V[i]->getNnz(); ++j )
            _rows[i]->set( V[i]->getIndexOfEntry( j ), V[i]->getValueOfEntry( j ) );
    }
}

void SparseUnsortedVectors::initializeToEmpty( unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVector( _n );
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows[i]" );
    }
}

double SparseUnsortedVectors::get( unsigned row, unsigned column ) const
{
    return _rows[row]->get( column );
}

void SparseUnsortedVectors::getRow( unsigned row, SparseVector *result ) const
{
    SparseUnsortedVector *unsortedRow = _rows[row];

    result->clear();
    for ( const auto &entry : *unsortedRow )
        result->commitChange( entry.first, entry.second );

    result->executeChanges();
}

const SparseUnsortedVector *SparseUnsortedVectors::getRow( unsigned row ) const
{
    return _rows[row];
}

void SparseUnsortedVectors::getRowDense( unsigned row, double *result ) const
{
    _rows[row]->toDense( result );
}

void SparseUnsortedVectors::getColumn( unsigned column, SparseVector *result ) const
{
    result->clear();

    for ( unsigned i = 0; i < _m; ++i )
        result->commitChange( i, _rows[i]->get( column ) );

    result->executeChanges();
}

void SparseUnsortedVectors::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void SparseUnsortedVectors::addLastRow( const double *row )
{
    SparseUnsortedVector **newRows = new SparseUnsortedVector *[_m + 1];
    if ( !newRows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::newRows" );

    memcpy( newRows, _rows, sizeof(SparseUnsortedVector *) * _m );

    newRows[_m] = new SparseUnsortedVector;
    if ( !newRows[_m] )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "ArrayOfCSRUnsortedVectors::newRows[_m]" );

    newRows[_m]->initialize( row, _n );

    delete[] _rows;
    _rows = newRows;

    ++_m;
}

void SparseUnsortedVectors::addLastColumn( const double *column )
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->addLastEntry( column[i] );

    ++_n;
}

void SparseUnsortedVectors::addEmptyColumn()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->incrementSize();

    ++_n;
}

void SparseUnsortedVectors::commitChange( unsigned row, unsigned column, double newValue )
{
    _rows[row]->commitChange( column, newValue );
}

void SparseUnsortedVectors::executeChanges()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->executeChanges();
}

void SparseUnsortedVectors::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    std::fill_n( numColumnElements, _n, 0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        // Rows
        numRowElements[i] = _rows[i]->getNnz();

        // Columns
        for ( const auto element : (*_rows[i]) )
            ++numColumnElements[element.first];
    }
}

void SparseUnsortedVectors::transposeIntoOther( SparseMatrix *other )
{
    other->initializeToEmpty( _n, _m );

    for ( unsigned row = 0; row < _m; ++row )
        for ( const auto &element : (*_rows[row]) )
            other->commitChange( element.first, row, element.second );

    other->executeChanges();
}

void SparseUnsortedVectors::dump() const
{
    dumpDense();
}

void SparseUnsortedVectors::dumpDense() const
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

void SparseUnsortedVectors::storeIntoOther( SparseMatrix *other ) const
{
    SparseUnsortedVectors *otherSparseUnsortedVectors = (SparseUnsortedVectors *)other;

    otherSparseUnsortedVectors->freeMemoryIfNeeded();

    otherSparseUnsortedVectors->_m = _m;
    otherSparseUnsortedVectors->_n = _n;

    otherSparseUnsortedVectors->_rows = new SparseUnsortedVector *[_m];
    if ( !otherSparseUnsortedVectors->_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::otherRows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        otherSparseUnsortedVectors->_rows[i] = new SparseUnsortedVector;
        if ( !otherSparseUnsortedVectors->_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::otherRows[i]" );

        (*otherSparseUnsortedVectors->_rows[i]) = (*_rows[i]);
    }
}

void SparseUnsortedVectors::mergeColumns( unsigned /* x1 */, unsigned /* x2 */ )
{
    throw BasisFactorizationError( BasisFactorizationError::FEATURE_NOT_YET_SUPPORTED,
                                   "SparseUnsortedVectors::mergeColumns" );
}

unsigned SparseUnsortedVectors::getNnz() const
{
    unsigned result = 0;
    for ( unsigned i = 0; i < _m; ++i )
        result += _rows[i]->getNnz();

    return result;
}

void SparseUnsortedVectors::toDense( double *result ) const
{
    std::fill_n( result, _n * _m, 0 );

    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->toDense( result + ( i * _n ) );
}

void SparseUnsortedVectors::clear()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->clear();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
