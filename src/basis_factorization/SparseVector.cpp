/*********************                                                        */
/*! \file SparseVector.cpp
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
#include "SparseVector.h"

SparseVector::SparseVector()
    : _size( 0 )
    , _estimatedNnz( 0 )
    , _nnz( 0 )
    , _values( NULL )
    , _indices( NULL )
    , _valuesA( NULL )
    , _valuesB( NULL )
    , _indicesA( NULL )
    , _indicesB( NULL )
    , _usingMemoryA( true )
{
}

SparseVector::SparseVector( unsigned size )
    : _values( NULL )
    , _indices( NULL )
    , _valuesA( NULL )
    , _valuesB( NULL )
    , _indicesA( NULL )
    , _indicesB( NULL )
{
    initializeToEmpty( size );
}

SparseVector::SparseVector( const double *V, unsigned size )
    : _values( NULL )
    , _indices( NULL )
    , _valuesA( NULL )
    , _valuesB( NULL )
    , _indicesA( NULL )
    , _indicesB( NULL )
{
    initialize( V, size );
}

SparseVector::~SparseVector()
{
    freeMemoryIfNeeded();
}

void SparseVector::initializeToEmpty( unsigned size )
{
    _size = size;
    _estimatedNnz = std::max( 2U, size / ROW_DENSITY_ESTIMATE );

    freeMemoryIfNeeded();

    ASSERT( !_valuesA );

    _valuesA = new double[_estimatedNnz];
    if ( !_valuesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesA" );

    _indicesA = new unsigned[_estimatedNnz];
    if ( !_indicesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesA" );

    _valuesB = new double[_estimatedNnz];
    if ( !_valuesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesB" );

    _indicesB = new unsigned[_estimatedNnz];
    if ( !_indicesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesB" );

    _values = _valuesA;
    _indices = _indicesA;
    _usingMemoryA = true;

    _nnz = 0;
}

void SparseVector::freeMemoryIfNeeded()
{
    if ( _valuesA )
    {
        delete[] _valuesA;
        _valuesA = NULL;
    }

    if ( _indicesA )
    {
        delete[] _indicesA;
        _indicesA = NULL;
    }

    if ( _valuesB )
    {
        delete[] _valuesB;
        _valuesB = NULL;
    }

    if ( _indicesB )
    {
        delete[] _indicesB;
        _indicesB = NULL;
    }

    _values = NULL;
    _indices = NULL;
}

void SparseVector::initialize( const double *V, unsigned size )
{
    initializeToEmpty( size );

    // Now populate the storage
    for ( unsigned i = 0; i < _size; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( V[i] ) )
            continue;

        if ( _nnz >= _estimatedNnz )
            increaseCapacity();

        _values[_nnz] = V[i];
        _indices[_nnz] = i;

        ++_nnz;
    }
}

void SparseVector::increaseCapacity()
{
    ASSERT( _size > 0 );

    unsigned newEstimatedNnz = _estimatedNnz + std::max( 2U, _size / ROW_DENSITY_ESTIMATE );

    double *newValuesA = new double[newEstimatedNnz];
    if ( !newValuesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::newValuesA" );

    unsigned *newIndicesA = new unsigned[newEstimatedNnz];
    if ( !newIndicesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::newIndicesA" );

    double *newValuesB = new double[newEstimatedNnz];
    if ( !newValuesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::newValuesB" );

    unsigned *newIndicesB = new unsigned[newEstimatedNnz];
    if ( !newIndicesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::newIndicesB" );

    memcpy( newValuesA, _values, _nnz * sizeof(double) );
    memcpy( newIndicesA, _indices, _nnz * sizeof(unsigned) );

    delete[] _valuesA;
    delete[] _indicesA;
    delete[] _valuesB;
    delete[] _indicesB;

    _valuesA = newValuesA;
    _indicesA = newIndicesA;
    _valuesB = newValuesB;
    _indicesB = newIndicesB;

    _usingMemoryA = true;
    _values = _valuesA;
    _indices = _indicesA;
    _estimatedNnz = newEstimatedNnz;
}

void SparseVector::clear()
{
    _nnz = 0;
}

unsigned SparseVector::getNnz() const
{
    return _nnz;
}

bool SparseVector::empty() const
{
    return _nnz == 0;
}

double SparseVector::get( unsigned entry ) const
{
    ASSERT( entry < _size );

    unsigned index = findArrayIndexForEntry( entry );
    return ( index == _nnz ) ? 0 : _values[index];
}

unsigned SparseVector::findArrayIndexForEntry( unsigned entry ) const
{
    if ( _nnz == 0 )
        return 0;

    int low = 0;
    int high = _nnz - 1;
    int mid;

    bool found = false;
    while ( !found && low <= high )
    {
        mid = ( low + high ) / 2;
        if ( _indices[mid] < entry )
            low = mid + 1;
        else if ( _indices[mid] > entry )
            high = mid - 1;
        else
            found = true;
    }

    return found ? mid : _nnz;
}

void SparseVector::dump() const
{
    printf( "\nDumping sparse vector: (nnz = %u)\n", _nnz );
    for ( unsigned i = 0; i < _nnz; ++i )
        printf( "\tEntry %u: %6.2lf\n", _indices[i], _values[i] );
    printf( "\n" );
}

void SparseVector::dumpDense() const
{
    printf( "\nDumping sparse vector: (nnz = %u)\n", _nnz );
    for ( unsigned i = 0; i < _size; ++i )
        printf( "\tEntry %u: %6.2lf\n", i, get( i ) );
    printf( "\n" );
}

void SparseVector::toDense( double *result ) const
{
    std::fill_n( result, _size, 0 );

    for ( unsigned i = 0; i < _nnz; ++i )
        result[_indices[i]] = _values[i];
}

SparseVector &SparseVector::operator=( const SparseVector &other )
{
    other.storeIntoOther( this );
    return *this;
}

void SparseVector::storeIntoOther( SparseVector *other ) const
{
    other->freeMemoryIfNeeded();

    other->_size = _size;
    other->_estimatedNnz = _estimatedNnz;
    other->_nnz = _nnz;

    other->_valuesA = new double[_estimatedNnz];
    if ( !other->_valuesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::otherValuesA" );

    other->_indicesA = new unsigned[_estimatedNnz];
    if ( !other->_indicesA )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::otherIndicesA" );

    other->_valuesB = new double[_estimatedNnz];
    if ( !other->_valuesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::otherValuesB" );

    other->_indicesB = new unsigned[_estimatedNnz];
    if ( !other->_indicesB )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::otherIndicesB" );

    other->_values = other->_valuesA;
    other->_indices = other->_indicesA;
    other->_usingMemoryA = true;

    memcpy( other->_values, _values, sizeof(double) * _nnz );
    memcpy( other->_indices, _indices, sizeof(unsigned) * _nnz );

    other->_committedChanges = _committedChanges;
}

unsigned SparseVector::getIndexOfEntry( unsigned entry ) const
{
    ASSERT( entry < _nnz );
    return _indices[entry];
}

double SparseVector::getValueOfEntry( unsigned entry ) const
{
    ASSERT( entry < _nnz );
    return _values[entry];
}

void SparseVector::addEmptyLastEntry()
{
    ++_size;
}

void SparseVector::addLastEntry( double value )
{
    if ( !FloatUtils::isZero( value ) )
    {
        if ( _nnz >= _estimatedNnz )
            increaseCapacity();

        _values[_nnz] = value;
        _indices[_nnz] = _size;

        ++_nnz;
    }

    ++_size;
}

void SparseVector::commitChange( unsigned index, double newValue )
{
    _committedChanges[index] = newValue;
}

void SparseVector::executeChanges()
{
    unsigned newEstimatedNnz = _estimatedNnz;
    bool needToIncreaseA = false;
    bool needToIncreaseB = false;
    if ( _committedChanges.size() + _nnz > _estimatedNnz )
    {
        newEstimatedNnz = _committedChanges.size() + _nnz;

        // We need a larger chunk of memory, to be on the safe side
        if ( _usingMemoryA )
        {
            ASSERT( _values == _valuesA );
            ASSERT( _indices == _indicesA );

            // Increase memory B
            delete[] _valuesB;
            delete[] _indicesB;

            _valuesB = new double[newEstimatedNnz];
            if ( !_valuesB )
                throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesB" );

            _indicesB = new unsigned[newEstimatedNnz];
            if ( !_indicesB )
                throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesB" );

            // Remember that we need to increase memory A, afterwards
            needToIncreaseA = true;
        }
        else
        {
            ASSERT( _values == _valuesB );
            ASSERT( _indices == _indicesB );

            // Increase memory A
            delete[] _valuesA;
            delete[] _indicesA;

            _valuesA = new double[newEstimatedNnz];
            if ( !_valuesA )
                throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesA" );

            _indicesA = new unsigned[newEstimatedNnz];
            if ( !_indicesA )
                throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesA" );

            // Remember that we need to increase memory B, afterwards
            needToIncreaseB = true;
        }
    }

    double *newValues = _usingMemoryA ? _valuesB : _valuesA;
    unsigned *newIndices = _usingMemoryA ? _indicesB : _indicesA;

    // Do one pass and populate the new arrays
    unsigned existingEntry = 0;
    unsigned newEntry = 0;
    Map<unsigned, double>::const_iterator change = _committedChanges.begin();

    ASSERT( newValues != _values );
    ASSERT( newIndices != _indices );

    bool done = false;
    while ( !done )
    {
        ASSERT( newEntry < newEstimatedNnz );

        if ( change == _committedChanges.end() )
        {
            // No more changes, copy remaining existing entries and be done
            ASSERT( newEntry < newEstimatedNnz );

            while ( existingEntry < _nnz )
            {
                newValues[newEntry] = _values[existingEntry];
                newIndices[newEntry] = _indices[existingEntry];
                ++newEntry;
                ++existingEntry;
            }

            done = true;
        }
        else if ( existingEntry == _nnz )
        {
            // No more existing entries, handle remaining changes and be done
            while ( change != _committedChanges.end() )
            {
                ASSERT( newEntry < newEstimatedNnz );

                if ( !FloatUtils::isZero( change->second ) )
                {
                    newValues[newEntry] = change->second;
                    newIndices[newEntry] = change->first;
                    ++newEntry;
                }

                ++change;
            }

            done = true;
        }
        else
        {
            // We have both existing entries and pending changes
            if ( _indices[existingEntry] == change->first )
            {
                // A change to an existing entry - change wins
                if ( !FloatUtils::isZero( change->second ) )
                {
                    // New value is not zero, store it
                    newValues[newEntry] = change->second;
                    newIndices[newEntry] = change->first;
                    ++newEntry;
                }

                ++change;
                ++existingEntry;
            }
            else if ( _indices[existingEntry] < change->first )
            {
                // Existing entry stays
                newValues[newEntry] = _values[existingEntry];
                newIndices[newEntry] = _indices[existingEntry];
                ++existingEntry;
                ++newEntry;
            }
            else
            {
                // Change is stored, assuming it is not zero
                if ( !FloatUtils::isZero( change->second ) )
                {
                    newValues[newEntry] = change->second;
                    newIndices[newEntry] = change->first;
                    ++newEntry;
                }

                ++change;
            }
        }
    }

    _values = newValues;
    _indices = newIndices;
    _nnz = newEntry;

    _usingMemoryA = !_usingMemoryA;

    if ( _usingMemoryA )
    {
        ASSERT( _values == _valuesA );
        ASSERT( _indices == _indicesA );
    }
    else
    {
        ASSERT( _values == _valuesB );
        ASSERT( _indices == _indicesB );
    }

    if ( needToIncreaseA )
    {
        ASSERT( !_usingMemoryA );

        delete[] _valuesA;
        delete[] _indicesA;

        _valuesA = new double[newEstimatedNnz];
        if ( !_valuesA )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesA" );

        _indicesA = new unsigned[newEstimatedNnz];
        if ( !_indicesA )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesA" );
    }
    else if ( needToIncreaseB )
    {
        ASSERT( _usingMemoryA );

        delete[] _valuesB;
        delete[] _indicesB;

        _valuesB = new double[newEstimatedNnz];
        if ( !_valuesB )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::valuesB" );

        _indicesB = new unsigned[newEstimatedNnz];
        if ( !_indicesB )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseVector::indicesB" );
    }

    _estimatedNnz = newEstimatedNnz;
    _committedChanges.clear();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
