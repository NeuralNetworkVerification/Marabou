/*********************                                                        */
/*! \file SparseUnsortedArray.cpp
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
#include "SparseUnsortedArray.h"
#include "SparseUnsortedList.h"

SparseUnsortedArray::SparseUnsortedArray()
    : _array( NULL )
    , _maxSize( 0 )
    , _allocatedSize( 0 )
    , _nnz( 0 )
{
}

SparseUnsortedArray::SparseUnsortedArray( unsigned size )
    : _array( NULL )
    , _maxSize( size )
    , _allocatedSize( 0 )
    , _nnz( 0 )
{
}

SparseUnsortedArray::SparseUnsortedArray( const double *V, unsigned size )
    : _array( NULL )
    , _maxSize( 0 )
    , _allocatedSize( 0 )
    , _nnz( 0 )
{
    initialize( V, size );
}

SparseUnsortedArray::~SparseUnsortedArray()
{
    freeMemoryIfNeeded();
}

void SparseUnsortedArray::freeMemoryIfNeeded()
{
    if ( _array )
    {
        delete[] _array;
        _array = NULL;
    }
}

void SparseUnsortedArray::initialize( const double *V, unsigned size )
{
    freeMemoryIfNeeded();

    _maxSize = size;

    _array = new Entry[CHUNK_SIZE];
    _allocatedSize = CHUNK_SIZE;
    _nnz = 0;

    for ( unsigned i = 0; i < _maxSize; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( V[i] ) )
            continue;

        if ( _allocatedSize == _nnz )
            increaseCapacity();

        _array[_nnz] = Entry( i, V[i] );
        ++_nnz;
    }
}

void SparseUnsortedArray::initializeFromList( const SparseUnsortedList *list )
{
    freeMemoryIfNeeded();

    _maxSize = list->getSize();

    _allocatedSize = list->getNnz();
    _array = new Entry[_allocatedSize];
    _nnz = 0;

    for ( auto it = list->begin(); it != list->end(); ++it )
    {
        _array[_nnz] = Entry( it->_index, it->_value );
        ++_nnz;
    }
}

void SparseUnsortedArray::clear()
{
    _nnz = 0;
}

unsigned SparseUnsortedArray::getNnz() const
{
    return _nnz;
}

bool SparseUnsortedArray::empty() const
{
    return _nnz == 0;
}

double SparseUnsortedArray::get( unsigned entry ) const
{
    for ( unsigned i = 0; i < _nnz; ++i )
    {
        if ( _array[i]._index == entry )
            return _array[i]._value;
    }

    return 0;
}

SparseUnsortedArray::Entry SparseUnsortedArray::getByArrayIndex( unsigned index ) const
{
    return _array[index];
}

void SparseUnsortedArray::dump() const
{
    printf( "\nDumping otherSparseUnsortedList: (nnz = %u)\n", _nnz );
    for ( unsigned i = 0; i < _nnz; ++i )
        printf( "\tEntry %u: %6.2lf\n", _array[i]._index, _array[i]._value );
    printf( "\n" );
}

void SparseUnsortedArray::dumpDense() const
{
    double *dense = new double[_maxSize];
    toDense( dense );

    for ( unsigned i = 0; i < _maxSize; ++i )
        printf( "%6.3lf ", dense[i] );

    printf( "\t(nnz = %u)", _nnz );

    delete[] dense;
}

void SparseUnsortedArray::toDense( double *result ) const
{
    std::fill_n( result, _maxSize, 0 );

    for ( unsigned i = 0; i < _nnz; ++i )
        result[_array[i]._index] = _array[i]._value;
}

SparseUnsortedArray &SparseUnsortedArray::operator=( const SparseUnsortedArray &other )
{
    freeMemoryIfNeeded();

    _maxSize = other._maxSize;
    _allocatedSize = other._allocatedSize;
    _nnz = other._nnz;

    _array = new Entry[_allocatedSize];

    memcpy( _array, other._array, sizeof(Entry) * _nnz );

    return *this;
}

void SparseUnsortedArray::storeIntoOther( SparseUnsortedArray *other ) const
{
    *other = *this;
}

void SparseUnsortedArray::set( unsigned index, double value )
{
    bool isZero = FloatUtils::isZero( value );

    for ( unsigned i = 0; i < _nnz; ++i )
    {
        if ( _array[i]._index == index )
        {
            if ( !isZero )
            {
                _array[i]._value = value;
            }
            else
            {
                _array[i] = _array[_nnz - 1];
                --_nnz;
            }

            return;
        }
    }

    if ( !isZero )
        append( index, value );
}

void SparseUnsortedArray::append( unsigned index, double value )
{
    if ( _allocatedSize == _nnz )
        increaseCapacity();

    _array[_nnz] = Entry( index, value );
    ++_nnz;
}

void SparseUnsortedArray::addLastEntry( double entry )
{
    if ( !FloatUtils::isZero( entry ) )
         append( _maxSize, entry );
    ++_maxSize;
}

void SparseUnsortedArray::erase( unsigned index )
{
    ASSERT( index < _nnz );

    _array[index] = _array[_nnz - 1];
    --_nnz;
}

void SparseUnsortedArray::mergeEntries( unsigned source, unsigned target )
{
    bool sourceFound = false;
    bool targetFound = false;

    unsigned sourceIndex = 0;
    unsigned targetIndex = 0;

    unsigned i = 0;
    while ( i < _nnz && !( sourceFound && targetFound ) )
    {
        if ( _array[i]._index == source )
        {
            sourceFound = true;
            sourceIndex = i;
        }
        else if ( _array[i]._index == target )
        {
            targetFound = true;
            targetIndex = i;
        }

        ++i;
    }

    // If no source entry exists, we are done
    if ( !sourceFound )
        return;

    // If no target entry, simply change index on source entry
    if ( !targetFound )
    {
        _array[sourceIndex]._index = target;
        return;
    }

    // Both source and target entries exist
    _array[targetIndex]._value += _array[sourceIndex]._value;

    // Delete source
    _array[sourceIndex] = _array[_nnz - 1];
    --_nnz;

    // Delete target, if needed
    if ( FloatUtils::isZero( _array[targetIndex]._value ) )
    {
        _array[targetIndex] = _array[_nnz - 1];
        --_nnz;
    }
}

void SparseUnsortedArray::incrementSize()
{
    ++_maxSize;
}

void SparseUnsortedArray::increaseCapacity()
{
    Entry *newArray = new Entry[_allocatedSize + CHUNK_SIZE];
    memcpy( newArray, _array, sizeof(Entry) * _nnz );
    delete[] _array;
    _array = newArray;
    _allocatedSize += CHUNK_SIZE;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
