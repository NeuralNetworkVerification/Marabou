/*********************                                                        */
/*! \file CSRMatrix.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "CSRMatrix.h"
#include "FloatUtils.h"
#include "MString.h"

CSRMatrix::CSRMatrix()
    : _m( 0 )
    , _n( 0 )
    , _A( NULL )
    , _IA( NULL )
    , _JA( NULL )
    , _nnz( 0 )
    , _estimatedNnz( 0 )
{
}

CSRMatrix::CSRMatrix( const double *M, unsigned m, unsigned n )
    : _m( 0 )
    , _n( 0 )
    , _A( NULL )
    , _IA( NULL )
    , _JA( NULL )
    , _nnz( 0 )
    , _estimatedNnz( 0 )
{
    initialize( M, m, n );
}

CSRMatrix::~CSRMatrix()
{
    freeMemoryIfNeeded();
}

void CSRMatrix::initialize( const double *M, unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    unsigned estimatedNumRowEntries = std::max( 2U, _n / ROW_DENSITY_ESTIMATE );
    _estimatedNnz = estimatedNumRowEntries * _m;

    _A = new double[_estimatedNnz];
    if ( !_A )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::A" );

    _IA = new unsigned[_m + 1];
    if ( !_IA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::IA" );

    _JA = new unsigned[_estimatedNnz];
    if ( !_JA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::JA" );

    // Now go over M, populate the arrays and find nnz
    _nnz = 0;
    _IA[0] = 0;
    for ( unsigned i = 0; i < _m; ++i )
    {
        _IA[i + 1] = _IA[i];
        for ( unsigned j = 0; j < _n; ++j )
        {
            // Ignore zero entries
            if ( FloatUtils::isZero( M[i*_n + j] ) )
                continue;

            if ( _nnz >= _estimatedNnz )
                increaseCapacity();

            _A[_nnz] = M[i*_n + j];
            ++_IA[i + 1];
            _JA[_nnz] = j;

            ++_nnz;
        }
    }
}

void CSRMatrix::increaseCapacity()
{
    unsigned estimatedNumRowEntries = std::max( 2U, _n / ROW_DENSITY_ESTIMATE );
    unsigned newEstimatedNnz = _estimatedNnz + ( estimatedNumRowEntries * _m );

    double *newA = new double[newEstimatedNnz];
    if ( !newA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::newA" );

    unsigned *newJA = new unsigned[newEstimatedNnz];
    if ( !newJA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::newJA" );

    memcpy( newA, _A, _estimatedNnz * sizeof(double) );
    memcpy( newJA, _JA, _estimatedNnz * sizeof(unsigned) );

    delete[] _A;
    delete[] _JA;

    _A = newA;
    _JA = newJA;
    _estimatedNnz = newEstimatedNnz;
}

double CSRMatrix::get( unsigned row, unsigned column ) const
{
    /*
      Elements of row i are stored in _A and _JA between
      indices _IA[i] and _IA[i+1] - 1. Perform binary search to
      look for the correct column index.
    */

    int low = _IA[row];
    int high = _IA[row + 1] - 1;
    int mid;
    while ( low <= high )
    {
        mid = ( low + high ) / 2;
        if ( _JA[mid] < column )
            low = mid + 1;
        else if ( _JA[mid] > column )
            high = mid - 1;
        else
            return _A[mid];
    }

    // Column doesn't exist, so element is 0
    return 0;
}

void CSRMatrix::addLastRow( double *row )
{
    // Array _IA needs to increase by one
    unsigned *newIA = new unsigned[_m + 2];
    memcpy( newIA, _IA, sizeof(unsigned) * ( _m + 1 ) );
    delete[] _IA;
    _IA = newIA;

    // Add the new row
    _IA[_m + 1] = _IA[_m];
    for ( unsigned i = 0; i < _n; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( row[i] ) )
            continue;

        if ( _nnz >= _estimatedNnz )
            increaseCapacity();

        _A[_nnz] = row[i];
        ++_IA[_m + 1];
        _JA[_nnz] = i;

        ++_nnz;
    }

    ++_m;
}

void CSRMatrix::freeMemoryIfNeeded()
{
    if ( _A )
    {
        delete[] _A;
        _A = NULL;
    }

    if ( _IA )
    {
        delete[] _IA;
        _IA = NULL;
    }

    if ( _JA )
    {
        delete[] _JA;
        _JA = NULL;
    }
}

void CSRMatrix::storeIntoOther( CSRMatrix *other ) const
{
    other->freeMemoryIfNeeded();

    other->_m = _m;
    other->_n = _n;
    other->_nnz = _nnz;
    other->_estimatedNnz = _estimatedNnz;

    other->_A = new double[_estimatedNnz];
    if ( !other->_A )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherA" );
    memcpy( other->_A, _A, sizeof(double) * _estimatedNnz );

    other->_IA = new unsigned[_m + 1];
    if ( !other->_IA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherIA" );
    memcpy( other->_IA, _IA, sizeof(unsigned) * ( _m + 1 ) );

    other->_JA = new unsigned[_estimatedNnz];
    if ( !other->_JA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherJA" );
    memcpy( other->_JA, _JA, sizeof(unsigned) * _estimatedNnz );
}

void CSRMatrix::getRow( unsigned row, double *result ) const
{
    std::fill_n( result, _n, 0.0 );

    /*
      Elements of row j are stored in _A and _JA between
      indices _IA[j] and _IA[j+1] - 1.
    */

    for ( unsigned i = _IA[row]; i < _IA[row + 1]; ++i )
        result[_JA[i]] = _A[i];
}

void CSRMatrix::getColumn( unsigned column, double *result ) const
{
    std::fill_n( result, _m, 0.0 );
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void CSRMatrix::mergeColumns( unsigned x1, unsigned x2 )
{
    List<unsigned> markedForDeletion;
    for ( unsigned i = 0; i < _m; ++i )
    {
        // Find the entry of x2
        int low = _IA[i];
        int high = _IA[i + 1] - 1;
        int mid;
        bool foundX2 = false;
        while ( !foundX2 && ( low <= high ) )
        {
            mid = ( low + high ) / 2;
            if ( _JA[mid] < x2 )
                low = mid + 1;
            else if ( _JA[mid] > x2 )
                high = mid - 1;
            else
                foundX2 = true;
        }

        /*
          If the loop terminated because x2 has a
          zero entry for this row, skip the row
        */
        if ( !foundX2 )
            continue;
        int x2Index = mid;

        // Now find the entry of x1
        low = _IA[i];
        high = _IA[i + 1] - 1;
        bool foundX1 = false;
        while ( !foundX1 && ( low <= high ) )
        {
            mid = ( low + high ) / 2;
            if ( _JA[mid] < x1 )
                low = mid + 1;
            else if ( _JA[mid] > x1 )
                high = mid - 1;
            else
                foundX1 = true;
        }

        if ( foundX1 )
        {
            /*
              If x1 already has an entry for this row,
              we adjust it, and mark x2's entry for deletion.
              The only exception is when the sum is zero, and both
              of them need to be deleted.
            */
            _A[mid] += _A[x2Index];
            if ( FloatUtils::isZero( _A[mid] ) )
                markedForDeletion.append( mid );

            markedForDeletion.append( x2Index );
        }
        else
        {
            /*
              x1 didn't have an entry, so we can re-use x2's
              entry instead of deleting it. However, we may need
              to re-sort the entries for this row.
            */
            _JA[x2Index] = x1;

            /*
              Elements of row i are stored in _A and _JA between
              indices _IA[i] and _IA[i+1] - 1. See whether the
              entry in question needs to move left or right.
            */
            while ( ( x2Index > (int)_IA[i] ) && ( x1 < _JA[x2Index - 1] ) )
            {
                _JA[x2Index] = _JA[x2Index - 1];
                _JA[x2Index - 1] = x1;
                --x2Index;
            }
            while ( ( x2Index < (int)_IA[i + 1] - 1 ) && ( x1 > _JA[x2Index + 1] ) )
            {
                _JA[x2Index] = _JA[x2Index + 1];
                _JA[x2Index + 1] = x1;
                ++x2Index;
            }
        }
    }

    // Finally, remove the entries that were marked for deletion.
    auto deletedEntry = markedForDeletion.begin();
    unsigned totalDeleted = 0;
    for ( unsigned i = 1; i < _m + 1; ++i )
    {
        // Count number of deleted entries in row i - 1
        unsigned deletedInThisRow = 0;
        while ( ( *deletedEntry < _IA[i] ) && ( deletedEntry != markedForDeletion.end() ) )
        {
            ++deletedInThisRow;
            ++deletedEntry;
        }

        _IA[i] -= ( deletedInThisRow + totalDeleted );
        totalDeleted += deletedInThisRow;
    }

    deletedEntry = markedForDeletion.begin();
    unsigned index = 0;
    unsigned copyToIndex = 0;
    while ( index < _nnz )
    {
        if ( index == *deletedEntry )
        {
            // We've hit another deleted entry.
            ++deletedEntry;
        }
        else
        {
            // This entry needs to stay. Copy if needed
            if ( index != copyToIndex )
            {
                _A[copyToIndex] = _A[index];
                _JA[copyToIndex] = _JA[index];
            }

            ++copyToIndex;
        }

        ++index;
    }

    _nnz -= markedForDeletion.size();
    --_n;
}

unsigned CSRMatrix::getNnz() const
{
    return _nnz;
}

void CSRMatrix::dump() const
{
    printf( "\nDumping internal arrays:\n" );

    printf( "\tA: " );
    for ( unsigned i = 0; i < _nnz; ++i )
        printf( "%5.2lf ", _A[i] );
    printf( "\n" );

    printf( "\tIA: " );
    for ( unsigned i = 0; i < _m + 1; ++i )
        printf( "%5u ", _IA[i] );
    printf( "\n" );

    printf( "\tJA: " );
    for ( unsigned i = 0; i < _nnz; ++i )
        printf( "%5u ", _JA[i] );
    printf( "\n" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
