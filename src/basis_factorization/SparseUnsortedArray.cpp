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

SparseUnsortedArray::SparseUnsortedArray()
    : _array( NULL )
    , _maxSize( 0 )
    , _allocatedSize( 0 )
    , _usedSize( 0 )
{
}

SparseUnsortedArray::SparseUnsortedArray( unsigned size )
    : _array( NULL )
    , _maxSize( size )
    , _allocatedSize( 0 )
    , _usedSize( 0 )
{
}

SparseUnsortedArray::SparseUnsortedArray( const double *V, unsigned size )
    : _array( NULL )
    , _maxSize( 0 )
    , _allocatedSize( 0 )
    , _usedSize( 0 )
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
    _maxSize = size;

    _array = new Entry[CHUNK_SIZE];
    _allocatedSize = CHUNK_SIZE;

    for ( unsigned i = 0; i < _maxSize; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( V[i] ) )
            continue;

        if ( _allocatedSize == _usedSize )
            increaseCapacity();

        _array[_usedSize] = Entry( i, V[i] );
        ++_usedSize;
    }
}

void SparseUnsortedArray::clear()
{
    _usedSize = 0;
}

unsigned SparseUnsortedArray::getNnz() const
{
    return _usedSize;
}

bool SparseUnsortedArray::empty() const
{
    return _usedSize == 0;
}

double SparseUnsortedArray::get( unsigned entry ) const
{
    for ( unsigned i = 0; i < _usedSize; ++i )
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
    printf( "\nDumping otherSparseUnsortedList: (nnz = %u)\n", _usedSize );
    for ( unsigned i = 0; i < _usedSize; ++i )
        printf( "\tEntry %u: %6.2lf\n", _array[i]._index, _array[i]._value );
    printf( "\n" );
}

void SparseUnsortedArray::dumpDense() const
{
    double *dense = new double[_maxSize];
    toDense( dense );

    for ( unsigned i = 0; i < _maxSize; ++i )
        printf( "%6.3lf ", dense[i] );

    delete[] dense;
}

void SparseUnsortedArray::toDense( double *result ) const
{
    std::fill_n( result, _maxSize, 0 );

    for ( unsigned i = 0; i < _usedSize; ++i )
        result[_array[i]._index] = _array[i]._value;
}

SparseUnsortedArray &SparseUnsortedArray::operator=( const SparseUnsortedArray &other )
{
    freeMemoryIfNeeded();

    _maxSize = other._maxSize;
    _allocatedSize = other._allocatedSize;
    _usedSize = other._usedSize;

    _array = new Entry[_allocatedSize];

    memcpy( _array, other._array, sizeof(Entry) * _usedSize );

    return *this;
}

void SparseUnsortedArray::storeIntoOther( SparseUnsortedArray *other ) const
{
    *other = *this;
}

void SparseUnsortedArray::set( unsigned index, double value )
{
    for ( unsigned i = 0; i < _usedSize; ++i )
    {
        if ( _array[i]._index == index )
        {
            _array[i]._value = value;
            return;
        }
    }

    if ( FloatUtils::isZero( value ) )
        return;

    append( index, value );
}

void SparseUnsortedArray::append( unsigned index, double value )
{
    if ( FloatUtils::isZero( value ) )
        return;

    if ( _allocatedSize == _usedSize )
        increaseCapacity();

    _array[_usedSize] = Entry( index, value );
    ++_usedSize;
}

void SparseUnsortedArray::addLastEntry( double entry )
{
    append( _maxSize, entry );
    ++_maxSize;
}

void SparseUnsortedArray::incrementSize()
{
    ++_maxSize;
}

// void SparseUnsortedArray::mergeEntries( unsigned source, unsigned target )
// {
//     List<Entry>::iterator sourceIt = _list.end();
//     List<Entry>::iterator targetIt = _list.end();
//     List<Entry>::iterator it;

//     for ( it = _list.begin(); it != _list.end(); ++it )
//     {
//         if ( it->_index == source )
//         {
//             sourceIt = it;
//             if ( targetIt != _list.end() )
//                 break;
//         }

//         if ( it->_index == target )
//         {
//             targetIt = it;
//             if ( sourceIt != _list.end() )
//                 break;
//         }
//     }

//     // If no source entry exists, we are done
//     if ( sourceIt == _list.end() )
//         return;

//     // If no target entry, simply change index on source entry
//     if ( targetIt == _list.end() )
//     {
//         sourceIt->_index = target;
//         return;
//     }

//     // Both source and target entries
//     targetIt->_value += sourceIt->_value;

//     _list.erase( sourceIt );
//     if ( FloatUtils::isZero( targetIt->_value ) )
//          _list.erase( targetIt );
// }

// List<SparseUnsortedArray::Entry>::iterator SparseUnsortedArray::erase( List<SparseUnsortedArray::Entry>::iterator it )
// {
//     return _list.erase( it );
// }

void SparseUnsortedArray::increaseCapacity()
{
    Entry *newArray = new Entry[_allocatedSize + CHUNK_SIZE];
    memcpy( newArray, _array, sizeof(Entry) * _usedSize );
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
