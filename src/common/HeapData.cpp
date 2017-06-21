/*********************                                                        */
/*! \file HeapData.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "CommonError.h"
#include "ConstSimpleData.h"
#include "HeapData.h"
#include "T/stdlib.h"
#include <string.h>

HeapData::HeapData() : _data( 0 ), _size( 0 )
{
}

HeapData::HeapData( void *data, unsigned size ) : _data( 0 ), _size( 0 )
{
    allocateMemory( size );
    memcpy( _data, data, size );
}

HeapData::HeapData( HeapData &other ) : _data( 0 ), _size( 0 )
{
    ( *this )+=other;
}

HeapData::HeapData( const ConstSimpleData &constSimpleData )
{
    allocateMemory( constSimpleData.size() );
    memcpy( _data, constSimpleData.data(), constSimpleData.size() );
}

HeapData::HeapData( const HeapData &other )
{
    allocateMemory( other.size() );
    memcpy( _data, other.data(), other.size() );
}

HeapData::~HeapData()
{
    freeMemoryIfNeeded();
}

HeapData &HeapData::operator=( const ConstSimpleData &data )
{
    freeMemoryIfNeeded();
    allocateMemory( data.size() );
    memcpy( _data, data.data(), data.size() );
    return *this;
}

HeapData &HeapData::operator=( const HeapData &other )
{
    freeMemoryIfNeeded();
    allocateMemory( other.size() );
    memcpy( _data, other.data(), other.size() );
    return *this;
}

HeapData &HeapData::operator+=( const ConstSimpleData &data )
{
    addMemory( data.size() );
    copyNewData( data.data(), data.size() );
    adjustSize( data.size() );

    return *this;
}

HeapData &HeapData::operator+=( const HeapData &data )
{
    ( *this )+=( ConstSimpleData( data ) );
    return *this;
}

bool HeapData::operator==( const HeapData &other ) const
{
    if ( _size != other._size )
        return false;

    return ( memcmp( _data, other._data, _size ) == 0 );
}

bool HeapData::operator!=( const HeapData &other ) const
{
    return !( *this == other );
}

bool HeapData::operator<( const HeapData &other ) const
{
    return
        ( _size < other._size ) ||
        ( ( _size == other._size ) && ( memcmp( _data, other._data, _size ) < 0 ) );
}

void *HeapData::data()
{
    return _data;
}

const void *HeapData::data() const
{
    return _data;
}

unsigned HeapData::size() const
{
    return _size;
}

void HeapData::clear()
{
    freeMemoryIfNeeded();
}

const char *HeapData::asChar() const
{
    return (char *)_data;
}


bool HeapData::allocated() const
{
    return _data != NULL;
}

void HeapData::freeMemory()
{
    T::free( _data );
    _data = NULL;
    _size = 0;
}

void HeapData::freeMemoryIfNeeded()
{
    if ( allocated() )
        freeMemory();
}

void HeapData::allocateMemory( unsigned size )
{
    _size = size;

    if ( ( _data = T::malloc( _size ) ) == NULL )
        throw CommonError( CommonError::NOT_ENOUGH_MEMORY );
}

void HeapData::addMemory( unsigned size )
{
    if ( ( _data = T::realloc( _data, _size + size ) ) == NULL )
        throw CommonError( CommonError::NOT_ENOUGH_MEMORY );
}

void HeapData::copyNewData( const void *newData, unsigned size )
{
    memcpy( (char *)_data + _size, newData, size );
}

void HeapData::adjustSize( unsigned size )
{
    _size += size;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
