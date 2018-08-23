/*********************                                                        */
/*! \file CSRMatrix.cpp
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
#include "CSRMatrix.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "MString.h"
#include "SparseVector.h"

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
    initializeToEmpty( m, n );

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

void CSRMatrix::initializeToEmpty( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    unsigned estimatedNumRowEntries = std::max( 2U, _n / ROW_DENSITY_ESTIMATE );
    _estimatedNnz = estimatedNumRowEntries * _m;

    freeMemoryIfNeeded();

    _A = new double[_estimatedNnz];
    if ( !_A )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::A" );

    _IA = new unsigned[_m + 1];
    if ( !_IA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::IA" );

    _JA = new unsigned[_estimatedNnz];
    if ( !_JA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::JA" );

    std::fill_n( _IA, _m + 1, 0.0 );
    _nnz = 0;
}

void CSRMatrix::increaseCapacity()
{
    ASSERT( _m > 0 && _n > 0 );

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
    unsigned index = findArrayIndexForEntry( row, column );

    // Return 0 if element is not found
    return ( index == _nnz ) ? 0 : _A[index];
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

void CSRMatrix::addLastColumn( double *column )
{
    // Count the number of new entries needed
    unsigned newNnz = 0;
    for ( unsigned i = 0; i < _m; ++i )
        if ( !FloatUtils::isZero( column[i] ) )
            ++newNnz;

    // Increase the size of _A and _JA if needed
    while ( _nnz + newNnz > _estimatedNnz )
        increaseCapacity();

    /*
      Now do a single sweep of the vectors from right
      to left, copy elements to their new locations and
      add the new elements where needed.
    */
    unsigned arrayIndex = _nnz - 1;
    unsigned offset = newNnz;
    for ( int i = _m - 1; i >= 0; --i )
    {
        if ( FloatUtils::isZero( column[i] ) )
            continue;

        // Ignore all rows greater than i
        while ( arrayIndex > _IA[i+1] - 1 )
        {
            _A[arrayIndex + offset] = _A[arrayIndex];
            _JA[arrayIndex + offset] = _JA[arrayIndex];
            --arrayIndex;
        }

        // We've entered our row. We are adding the last column
        _A[arrayIndex + offset] = column[i];
        _JA[arrayIndex + offset] = _n;
        --offset;
    }

    // Update the row counters
    unsigned increase = 0;
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::isZero( column[i] ) )
            ++increase;
        _IA[i+1] += increase;
    }

    ++_n;
    _nnz += newNnz;
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

void CSRMatrix::storeIntoOther( SparseMatrix *other ) const
{
    CSRMatrix *otherCsr = (CSRMatrix *)other;

    otherCsr->freeMemoryIfNeeded();

    otherCsr->_m = _m;
    otherCsr->_n = _n;
    otherCsr->_nnz = _nnz;
    otherCsr->_estimatedNnz = _estimatedNnz;

    otherCsr->_A = new double[_estimatedNnz];
    if ( !otherCsr->_A )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherCsrA" );
    memcpy( otherCsr->_A, _A, sizeof(double) * _estimatedNnz );

    otherCsr->_IA = new unsigned[_m + 1];
    if ( !otherCsr->_IA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherCsrIA" );
    memcpy( otherCsr->_IA, _IA, sizeof(unsigned) * ( _m + 1 ) );

    otherCsr->_JA = new unsigned[_estimatedNnz];
    if ( !otherCsr->_JA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "CSRMatrix::otherCsrJA" );
    memcpy( otherCsr->_JA, _JA, sizeof(unsigned) * _estimatedNnz );
}

void CSRMatrix::getRow( unsigned row, SparseVector *result ) const
{
    ASSERT( row < _m );

    /*
      Elements of row j are stored in _A and _JA between
      indices _IA[j] and _IA[j+1] - 1.
    */
    result->clear();
    for ( unsigned i = _IA[row]; i < _IA[row + 1]; ++i )
        result->commitChange( _JA[i], _A[i] );
    result->executeChanges();
}

void CSRMatrix::getRowDense( unsigned row, double *result ) const
{
    std::fill_n( result, _n, 0 );
    for ( unsigned i = _IA[row]; i < _IA[row + 1]; ++i )
        result[_JA[i]] = _A[i];
}

void CSRMatrix::getColumn( unsigned column, SparseVector *result ) const
{
    ASSERT( column < _n );

    result->clear();
    for ( unsigned i = 0; i < _m; ++i )
        result->commitChange( i, get( i, column ) );
    result->executeChanges();
}

void CSRMatrix::getColumnDense( unsigned column, double *result ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        result[i] = get( i, column );
}

void CSRMatrix::mergeColumns( unsigned x1, unsigned x2 )
{
    List<unsigned> markedForDeletion;
    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned x2Index = findArrayIndexForEntry( i, x2 );
        bool foundX2 = ( x2Index != _nnz );

        /*
          If x2 has a zero entry for this row, skip the row
        */
        if ( !foundX2 )
            continue;

        // Now find the entry of x1
        unsigned x1Index = findArrayIndexForEntry( i, x1 );
        bool foundX1 = ( x1Index != _nnz );

        if ( foundX1 )
        {
            /*
              If x1 already has an entry for this row,
              we adjust it, and mark x2's entry for deletion.
              The only exception is when the sum is zero, and both
              of them need to be deleted.
            */
            _A[x1Index] += _A[x2Index];
            if ( FloatUtils::isZero( _A[x1Index] ) )
                markedForDeletion.append( x1Index );

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

            double temp = _A[x2Index];
            while ( ( x2Index > _IA[i] ) && ( x1 < _JA[x2Index - 1] ) )
            {
                _JA[x2Index] = _JA[x2Index - 1];
                _JA[x2Index - 1] = x1;
                _A[x2Index] = _A[x2Index - 1];
                _A[x2Index - 1] = temp;
                --x2Index;
            }
            while ( ( x2Index < _IA[i + 1] - 1 ) && ( x1 > _JA[x2Index + 1] ) )
            {
                _JA[x2Index] = _JA[x2Index + 1];
                _JA[x2Index + 1] = x1;
                _A[x2Index] = _A[x2Index + 1];
                _A[x2Index + 1] = temp;
                ++x2Index;
            }
        }
    }

    // Finally, remove the entries that were marked for deletion.
    deleteElements( markedForDeletion );

    // Note that _n is not changed, because the merged column is not
    // deleted - rather, it just stays empty.
}

void CSRMatrix::deleteElements( const List<unsigned> &deletions )
{
    if ( deletions.empty() )
        return;

    auto deletedEntry = deletions.begin();
    unsigned totalDeleted = 0;
    for ( unsigned i = 1; i < _m + 1; ++i )
    {
        // Count number of deleted entries in row i - 1
        unsigned deletedInThisRow = 0;
        while ( ( deletedEntry != deletions.end() ) && ( *deletedEntry < _IA[i] ) )
        {
            ++deletedInThisRow;
            ++deletedEntry;
        }

        _IA[i] -= ( deletedInThisRow + totalDeleted );
        totalDeleted += deletedInThisRow;
    }

    deletedEntry = deletions.begin();
    unsigned index = 0;
    unsigned copyToIndex = 0;
    while ( index < _nnz )
    {
        if ( ( deletedEntry != deletions.end() ) && ( index == *deletedEntry ) )
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

    _nnz -= deletions.size();
}

void CSRMatrix::insertElements( const Map<unsigned, Set<CommittedChange>> &insertions )
{
    // Increase capacity if needed
    unsigned totalInsertions = 0;
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( insertions.exists( i ) )
            totalInsertions += insertions[i].size();
    }

    unsigned newNnz = _nnz + totalInsertions;
    while ( newNnz >= _estimatedNnz )
        increaseCapacity();

    // Traverse rows from last to first, add elements as needed
    int arrayIndex = _nnz - 1;
    int newArrayIndex = arrayIndex + totalInsertions;

    Set<CommittedChange>::const_reverse_iterator nextInsertion;
    for ( int i = _m - 1; i >= 0; --i )
    {
        bool rowHasInsertions = insertions.exists( i );
        if ( rowHasInsertions )
            nextInsertion = insertions[i].rbegin();

        /*
          Elements of row i are stored in _A and _JA between
          indices _IA[i] and _IA[i+1] - 1.
        */
        int j = _IA[i+1] - 1;
        while ( j >= (int)_IA[i] )
        {
            if ( !rowHasInsertions || nextInsertion == insertions[i].rend() )
            {
                // No more insertions here, just copy the element
                _A[newArrayIndex] = _A[j];
                _JA[newArrayIndex] = _JA[j];

                --j;
            }
            else
            {
                // We still have insertions for this row, but is this the right spot?
                if ( _JA[j] < nextInsertion->_column )
                {
                    _A[newArrayIndex] = nextInsertion->_value;
                    _JA[newArrayIndex] = nextInsertion->_column;

                    ++nextInsertion;
                }
                else
                {
                    _A[newArrayIndex] = _A[j];
                    _JA[newArrayIndex] = _JA[j];

                    --j;
                }
            }

            --newArrayIndex;
        }

        /*
          We are done processing the elements of this row, but there may be
          some insertions left, in which case we just insert them now
        */
        if ( rowHasInsertions )
        {
            while ( nextInsertion != insertions[i].rend() )
            {
                _A[newArrayIndex] = nextInsertion->_value;
                _JA[newArrayIndex] = nextInsertion->_column;
                ++nextInsertion;
                --newArrayIndex;
            }
        }

    }

    // Make a final pass to adjust the IA indices
    unsigned totalAddedSoFar = 0;
    for ( unsigned i = 0; i < _m; ++i )
    {
        totalAddedSoFar += insertions.exists( i ) ? insertions[i].size() : 0;
        _IA[i+1] += totalAddedSoFar;
    }

    _nnz += totalAddedSoFar;
}

void CSRMatrix::addEmptyColumn()
{
    ++_n;
}

unsigned CSRMatrix::getNnz() const
{
    return _nnz;
}

void CSRMatrix::toDense( double *result ) const
{
    std::fill_n( result, _m * _n, 0 );

    unsigned arrayIndex = 0;
    for ( unsigned row = 0; row < _m; ++row )
    {
        while ( arrayIndex < _IA[row + 1] )
        {
            result[row * _n + _JA[arrayIndex]] = _A[arrayIndex];
            ++arrayIndex;
        }
    }
}

void CSRMatrix::commitChange( unsigned row, unsigned column, double newValue )
{
    ASSERT( FloatUtils::wellFormed( newValue ) );

    // First check whether the entry already exists
    unsigned index = findArrayIndexForEntry( row, column );
    bool found = ( index < _nnz );

    if ( !found )
    {
        // Entry doesn't exist currently
        if ( !FloatUtils::isZero( newValue ) )
        {
            CommittedChange change;
            change._column = column;
            change._value = newValue;
            _committedInsertions[row].insert( change );
        }
    }
    else
    {
        // Entry currently exists
        if ( FloatUtils::isZero( newValue ) )
        {
            _committedDeletions.insert( index );
        }
        else if ( FloatUtils::areDisequal( newValue, _A[index] ) )
        {
            CommittedChange change;
            change._column = column;
            change._value = newValue;
            _committedChanges[row].append( change );
        }
    }
}

unsigned CSRMatrix::findArrayIndexForEntry( unsigned row, unsigned column ) const
{
    int low = _IA[row];
    int high = _IA[row + 1] - 1;
    int mid;

    bool found = false;
    while ( !found && low <= high )
    {
        mid = ( low + high ) / 2;
        if ( _JA[mid] < column )
            low = mid + 1;
        else if ( _JA[mid] > column )
            high = mid - 1;
        else
            found = true;
    }

    return found ? mid : _nnz;
}

void CSRMatrix::executeChanges()
{
    // Get the changes out of the way
    for ( const auto &changedRow : _committedChanges )
    {
        unsigned row = changedRow.first;
        for ( const auto &changedColumn : changedRow.second )
        {
            unsigned column = changedColumn._column;
            unsigned index = findArrayIndexForEntry( row, column );

            ASSERT( index < _nnz );

            _A[index] = changedColumn._value;
        }
    }
    _committedChanges.clear();

    // Next, handle the deletions. Do a pass on the data structures, and shrink
    // them as needed.
    if ( !_committedDeletions.empty() )
    {
        List<unsigned> deletions;
        for ( const auto &deletedEntry : _committedDeletions )
            deletions.append( deletedEntry );
        deleteElements( deletions );

        _committedDeletions.clear();
    }

    // Finally, handle the insertions
    if ( !_committedInsertions.empty() )
    {
        insertElements( _committedInsertions );
        _committedInsertions.clear();
    }
}

void CSRMatrix::countElements( unsigned *numRowElements, unsigned *numColumnElements )
{
    for ( unsigned i = 0; i < _m; ++i )
        numRowElements[i] = _IA[i+1] - _IA[i];

    std::fill_n( numColumnElements, _n, 0 );
    for ( unsigned i = 0; i < _nnz; ++i )
        ++numColumnElements[_JA[i]];
}

void CSRMatrix::transposeIntoOther( SparseMatrix *other )
{
    other->initializeToEmpty( _n, _m );

    unsigned row = 0;
    for ( unsigned i = 0; i < _nnz; ++i )
    {
        // Find the row of the element indexed i
        while ( i >= _IA[row + 1] )
            ++row;

        other->commitChange( _JA[i], row, _A[i] );
    }

    other->executeChanges();
}

void CSRMatrix::dump() const
{
    printf( "\nDumping internal arrays: (nnz = %u)\n", _nnz );

    printf( "\tA:\n" );
    unsigned row = 0;
    for ( unsigned i = 0; i < _nnz; ++i )
    {
        bool jump = false;

        while ( i == _IA[row] )
        {
            jump = true;
            ++row;
        }

        if ( jump )
            printf( "\n\t\t" );

        printf( "%5.2lf ", _A[i] );
    }

    printf( "\n" );

    printf( "\tJA: " );
    row = 0;
    for ( unsigned i = 0; i < _nnz; ++i )
    {
        bool jump = false;

        while ( i == _IA[row] )
        {
            jump = true;
            ++row;
        }

        if ( jump )
            printf( "\n\t\t" );

        printf( "%5u ", _JA[i] );
    }


    printf( "\n" );

    printf( "\tIA: " );
    for ( unsigned i = 0; i < _m + 1; ++i )
        printf( "%5u ", _IA[i] );
    printf( "\n" );

}

void CSRMatrix::dumpDense() const
{
    double *work = new double[_m * _n];
    toDense( work );

    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _n; ++j )
        {
            printf( "%5.2lf ", work[i*_n + j] );
        }
        printf( "\n" );
    }

    printf( "\n" );
    delete[] work;
}

void CSRMatrix::checkInvariants() const
{
    // Check that all _JA elements are within range
    for ( unsigned i = 0; i < _nnz; ++i )
    {
        if ( _JA[i] >= _n )
        {
            printf( "CSRMatrix error! Have a _JA element out of range. "
                    "Dumping and terminating\n" );
            dump();
            exit( 1 );
        }
    }

    // For each row, check that the JA entries are increasing
    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned start = _IA[i];
        unsigned end = _IA[i+1];

        for ( unsigned j = start; j + 1 < end; ++j )
        {
            if ( _JA[j] >= _JA[j+1] )
            {
                printf( "CSRMatrix error! _JA elements not increasing. "
                        "Dumping and terminating\n" );
                dump();
                exit( 1 );
            }
        }
    }
}

void CSRMatrix::clear()
{
    _nnz = 0;
    for ( unsigned i = 0; i < _m; ++i )
        _IA[i+1] = 0;
}

const double *CSRMatrix::getA() const
{
    return _A;
}

const unsigned *CSRMatrix::getJA() const
{
    return _JA;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
