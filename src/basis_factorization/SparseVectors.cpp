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

#include <cstring>
#include "BasisFactorizationError.h"
#include "SparseVectors.h"

SparseVectors::SparseVectors()
    : _rows( NULL )
    , _m( 0 )
    , _n( 0 )
{
}

SparseVectors::~SparseVectors()
{
    freeMemoryIfNeeded();
}

void SparseVectors::freeMemoryIfNeeded()
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

void SparseVectors::initialize( const double *M, unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _rows = new SparseVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseVector;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows[i]" );
        _rows[i]->initialize( M + ( _n * i ), _n );
    }
}

void SparseVectors::initialize( const SparseVector **V, unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _rows = new SparseVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseVector();
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows[i]" );
        V[i]->storeIntoOther( _rows[i] );
    }
}

void SparseVectors::initializeToEmpty( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _rows = new SparseVector *[_m];
    if ( !_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i] = new SparseVector;
        if ( !_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::rows[i]" );
        _rows[i]->initializeToEmpty( _n );
    }
}

double SparseVectors::get( unsigned row, unsigned column ) const
{
    return _rows[row]->get( column );
}

void SparseVectors::getRow( unsigned row, SparseVector *result ) const
{
    _rows[row]->storeIntoOther( result );
}

void SparseVectors::getRowDense( unsigned row, double *result ) const
{
    _rows[row]->toDense( result );
}

void SparseVectors::getColumn( unsigned column, SparseVector *result ) const
{
    result->clear();

    for ( unsigned i = 0; i < _m; ++i )
        result->commitChange( i, _rows[i]->get( column ) );

    result->executeChanges();
}

void SparseVectors::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = _rows[i]->get( column );
}

void SparseVectors::addLastRow( double *row )
{
    SparseVector **newRows = new SparseVector *[_m + 1];
    if ( !newRows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::newRows" );

    memcpy( newRows, _rows, sizeof(SparseVector *) * _m );

    newRows[_m] = new SparseVector( row, _n );
    if ( !newRows[_m] )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "ArrayOfCSRVectors::newRows[_m]" );

    delete[] _rows;
    _rows = newRows;

    ++_m;
}

void SparseVectors::addLastColumn( double *column )
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->addLastEntry( column[i] );

    ++_n;
}

void SparseVectors::addEmptyColumn()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->addEmptyLastEntry();
}

void SparseVectors::commitChange( unsigned row, unsigned column, double newValue )
{
    _rows[row]->commitChange( column, newValue );
}

void SparseVectors::executeChanges()
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->executeChanges();
}

void SparseVectors::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    std::fill_n( numColumnElements, _n, 0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned nnz = _rows[i]->getNnz();

        // Rows
        numRowElements[i] = nnz;

        // Columns
        const unsigned *ja = _rows[i]->getJA();
        for ( unsigned j = 0; j < nnz; ++j )
            ++numColumnElements[ja[j]];
    }
}

void SparseVectors::transposeIntoOther( SparseMatrix *other )
{
    other->initializeToEmpty( _n, _m );

    for ( unsigned row = 0; row < _m; ++row )
    {
        unsigned nnz = _rows[row]->getNnz();
        for ( unsigned i = 0; i < nnz; ++i )
        {
            unsigned column = _rows[row]->getIndexOfEntry( i );
            other->commitChange( column, row, _rows[row]->getValueOfEntry( i ) );
        }
    }

    other->executeChanges();
}

void SparseVectors::dump() const
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->dumpDense();

    printf( "\n" );
}

void SparseVectors::dumpDense() const
{
    dump();
}

void SparseVectors::storeIntoOther( SparseMatrix *other ) const
{
    SparseVectors *otherSparseVectors = (SparseVectors *)other;

    otherSparseVectors->freeMemoryIfNeeded();

    otherSparseVectors->_m = _m;
    otherSparseVectors->_n = _n;

    otherSparseVectors->_rows = new SparseVector *[_m];
    if ( !otherSparseVectors->_rows )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::otherRows" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        otherSparseVectors->_rows[i] = new SparseVector();
        if ( !otherSparseVectors->_rows[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVectors::otherRows[i]" );
        _rows[i]->storeIntoOther( otherSparseVectors->_rows[i] );
    }
}

void SparseVectors::mergeColumns( unsigned /* x1 */, unsigned /* x2 */ )
{
    throw BasisFactorizationError( BasisFactorizationError::FEATURE_NOT_YET_SUPPORTED,
                                   "SparseVectors::mergeColumns" );
}

unsigned SparseVectors::getNnz() const
{
    unsigned result = 0;
    for ( unsigned i = 0; i < _m; ++i )
        result += _rows[i]->getNnz();
    return result;
}

void SparseVectors::toDense( double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        _rows[i]->toDense( result + ( i * _n ) );
}

void SparseVectors::clear()
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
