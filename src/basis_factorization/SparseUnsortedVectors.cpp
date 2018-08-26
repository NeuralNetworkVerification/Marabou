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

    _rows = new SparseUnsortedVectors::Row *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVectors::Row;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows[i]" );

        for ( unsigned j = 0; j < _n; ++j )
        {
            double value = M[i*_n + j];
            if ( !FloatUtils::isZero( value ) )
                _rows[i]->set( j, value );
        }
    }
}

void SparseUnsortedVectors::initialize( const SparseVector **V, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _rows = new SparseUnsortedVectors::Row *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVectors::Row;
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

    _rows = new SparseUnsortedVectors::Row *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseUnsortedVectors::Row;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::rows[i]" );
    }
}

double SparseUnsortedVectors::get( unsigned row, unsigned column ) const
{
    return _rows[row]->exists( column ) ? _rows[row]->get( column ) : 0;
}

void SparseUnsortedVectors::getRow( unsigned row, SparseVector *result ) const
{
    Row *unsortedRow = _rows[row];

    result->clear();
    for ( const auto &entry : *unsortedRow )
        result->commitChange( entry.first, entry.second );

    result->executeChanges();
}

void SparseUnsortedVectors::getRowDense( unsigned row, double *result ) const
{
    Row *unsortedRow = _rows[row];

    std::fill_n( result, _n, 0 );
    for ( const auto &entry : *unsortedRow )
        result[entry.first] = entry.second;
}

void SparseUnsortedVectors::getColumn( unsigned column, SparseVector *result ) const
{
    result->clear();

    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( _rows[i]->exists( column ) )
            result->commitChange( i, _rows[i]->get( column ) );
    }

    result->executeChanges();
}

void SparseUnsortedVectors::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void SparseUnsortedVectors::addLastRow( double *row )
{
    SparseUnsortedVectors::Row **newRows = new SparseUnsortedVectors::Row *[_m + 1];
    if ( !newRows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::newRows" );

    memcpy( newRows, _rows, sizeof(SparseUnsortedVectors::Row *) * _m );

    newRows[_m] = new SparseUnsortedVectors::Row;
    if ( !newRows[_m] )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "ArrayOfCSRUnsortedVectors::newRows[_m]" );

    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( !FloatUtils::isZero( row[i] ) )
            newRows[_m]->set( i, row[i] );
    }

    delete[] _rows;
    _rows = newRows;

    ++_m;
}

void SparseUnsortedVectors::addLastColumn( double *column )
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::isZero( column[i] ) )
            _rows[i]->set( _n, column[i] );
    }

    ++_n;
}

void SparseUnsortedVectors::addEmptyColumn()
{
    ++_n;
}

void SparseUnsortedVectors::commitChange( unsigned row, unsigned column, double newValue )
{
    _committedChangesByRow[row].append( SparseUnsortedVectors::CommittedChange( column, newValue ) );
}

void SparseUnsortedVectors::executeChanges()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( const auto &change : _committedChangesByRow[i] )
        {
            double value = change._value;
            double column = change._index;

            if ( !FloatUtils::isZero( value ) )
            {
                _rows[i]->set( column, value );
            }
            else
            {
                if ( _rows[i]->exists( column ) )
                    _rows[i]->erase( column );
            }
        }

        _committedChangesByRow[i].clear();
    }
}

void SparseUnsortedVectors::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    std::fill_n( numColumnElements, _n, 0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned nnzInRow = _rows[i]->size();

        // Rows
        numRowElements[i] = nnzInRow;

        // Columns
        for ( const auto element : (*_rows[i]) )
            ++numColumnElements[element.first];
    }
}

void SparseUnsortedVectors::transposeIntoOther( SparseMatrix *other )
{
    other->initializeToEmpty( _n, _m );

    for ( unsigned row = 0; row < _m; ++row )
    {
        for ( const auto &element : (*_rows[row]) )
        {
            other->commitChange( element.first, row, element.second );
        }
    }

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
        for ( unsigned column = 0; column < _n; ++column )
        {
            printf( "%6.3lf ", get( row, column ) );
        }
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

    otherSparseUnsortedVectors->_rows = new SparseUnsortedVectors::Row *[_m];
    if ( !otherSparseUnsortedVectors->_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseUnsortedVectors::otherRows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        otherSparseUnsortedVectors->_rows[i] = new SparseUnsortedVectors::Row;
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
        result += _rows[i]->size();
    return result;
}

void SparseUnsortedVectors::toDense( double *result ) const
{
    std::fill_n( result, _n * _m, 0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        double *row = result + ( i * _n );
        for ( const auto element : (*_rows[i]) )
            row[element.first] = element.second;
    }
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
