/*********************                                                        */
/*! \file SparseUnsortedList.cpp
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
#include "SparseUnsortedList.h"

SparseUnsortedList::SparseUnsortedList()
    : _size( 0 )
{
}

SparseUnsortedList::SparseUnsortedList( unsigned size )
    : _size( size )
{
}

SparseUnsortedList::SparseUnsortedList( const double *V, unsigned size )
{
    initialize( V, size );
}

SparseUnsortedList::~SparseUnsortedList()
{
}

void SparseUnsortedList::initialize( const double *V, unsigned size )
{
    _size = size;
    _list.clear();

    for ( unsigned i = 0; i < _size; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( V[i] ) )
            continue;

        _list.append( Entry( i, V[i] ) );
    }
}

void SparseUnsortedList::clear()
{
    _list.clear();
}

unsigned SparseUnsortedList::getNnz() const
{
    return _list.size();
}

bool SparseUnsortedList::empty() const
{
    return _list.empty();
}

double SparseUnsortedList::get( unsigned entry ) const
{
    for ( const auto &listEntry : _list )
    {
        if ( listEntry._index == entry )
            return listEntry._value;
    }

    return 0;
}

void SparseUnsortedList::dump() const
{
    printf( "\nDumping sparse unsortedList: (nnz = %u)\n", _list.size() );
    for ( const auto &entry : _list )
        printf( "\tEntry %u: %6.2lf\n", entry._index, entry._value );
    printf( "\n" );
}

void SparseUnsortedList::dumpDense() const
{
    double *dense = new double[_size];
    toDense( dense );

    for ( unsigned i = 0; i < _size; ++i )
        printf( "%6.3lf ", dense[i] );

    delete[] dense;
}

void SparseUnsortedList::toDense( double *result ) const
{
    std::fill_n( result, _size, 0 );

    for ( const auto &entry : _list )
        result[entry._index] = entry._value;
}

SparseUnsortedList &SparseUnsortedList::operator=( const SparseUnsortedList &other )
{
    _size = other._size;
    _list = other._list;

    return *this;
}

void SparseUnsortedList::storeIntoOther( SparseUnsortedList *other ) const
{
    other->_size = _size;
    other->_list = _list;
}

List<SparseUnsortedList::Entry>::const_iterator SparseUnsortedList::begin() const
{
    return _list.begin();
}

List<SparseUnsortedList::Entry>::const_iterator SparseUnsortedList::end() const
{
    return _list.end();
}

List<SparseUnsortedList::Entry>::iterator SparseUnsortedList::begin()
{
    return _list.begin();
}

List<SparseUnsortedList::Entry>::iterator SparseUnsortedList::end()
{
    return _list.end();
}

void SparseUnsortedList::set( unsigned index, double value )
{
    bool isZero = FloatUtils::isZero( value );

    for ( auto it = begin(); it != end(); ++it )
    {
        if ( it->_index == index )
        {
            if ( isZero )
                _list.erase( it );
            else
                it->_value = value;

            return;
        }
    }

    if ( !isZero )
        _list.append( Entry( index, value ) );
}

void SparseUnsortedList::append( unsigned index, double value )
{
    _list.append( Entry( index, value ) );
}

void SparseUnsortedList::addLastEntry( double entry )
{
    if ( !FloatUtils::isZero( entry ) )
        _list.append( Entry( _size, entry ) );

    ++_size;
}

void SparseUnsortedList::incrementSize()
{
    ++_size;
}

void SparseUnsortedList::mergeEntries( unsigned source, unsigned target )
{
    List<Entry>::iterator sourceIt = _list.end();
    List<Entry>::iterator targetIt = _list.end();
    List<Entry>::iterator it;

    for ( it = _list.begin(); it != _list.end(); ++it )
    {
        if ( it->_index == source )
        {
            sourceIt = it;
            if ( targetIt != _list.end() )
                break;
        }

        if ( it->_index == target )
        {
            targetIt = it;
            if ( sourceIt != _list.end() )
                break;
        }
    }

    // If no source entry exists, we are done
    if ( sourceIt == _list.end() )
        return;

    // If no target entry, simply change index on source entry
    if ( targetIt == _list.end() )
    {
        sourceIt->_index = target;
        return;
    }

    // Both source and target entries
    targetIt->_value += sourceIt->_value;

    _list.erase( sourceIt );
    if ( FloatUtils::isZero( targetIt->_value ) )
         _list.erase( targetIt );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
