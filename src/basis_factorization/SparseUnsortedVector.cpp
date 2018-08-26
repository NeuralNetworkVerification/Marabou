/*********************                                                        */
/*! \file SparseUnsortedVector.cpp
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
#include "SparseUnsortedVector.h"

SparseUnsortedVector::SparseUnsortedVector()
    : _size( 0 )
{
}

SparseUnsortedVector::SparseUnsortedVector( unsigned size )
    : _size( size )
{
}

SparseUnsortedVector::SparseUnsortedVector( const double *V, unsigned size )
{
    initialize( V, size );
}

SparseUnsortedVector::~SparseUnsortedVector()
{
}

void SparseUnsortedVector::initialize( const double *V, unsigned size )
{
    _size = size;
    _vector.clear();

    for ( unsigned i = 0; i < _size; ++i )
    {
        // Ignore zero entries
        if ( FloatUtils::isZero( V[i] ) )
            continue;

        _vector[i] = V[i];
    }
}

void SparseUnsortedVector::clear()
{
    _vector.clear();
}

unsigned SparseUnsortedVector::getNnz() const
{
    return _vector.size();
}

bool SparseUnsortedVector::empty() const
{
    return _vector.empty();
}

double SparseUnsortedVector::get( unsigned entry ) const
{
    return _vector.exists( entry ) ? _vector.get( entry ) : 0;
}

void SparseUnsortedVector::dump() const
{
    printf( "\nDumping sparse unsortedVector: (nnz = %u)\n", _vector.size() );
    for ( const auto &entry : _vector )
        printf( "\tEntry %u: %6.2lf\n", entry.first, entry.second );
    printf( "\n" );
}

void SparseUnsortedVector::dumpDense() const
{
    for ( unsigned i = 0; i < _size; ++i )
        printf( "%6.3lf ", get( i ) );
}

void SparseUnsortedVector::toDense( double *result ) const
{
    std::fill_n( result, _size, 0 );

    for ( const auto &entry : _vector )
        result[entry.first] = entry.second;
}

SparseUnsortedVector &SparseUnsortedVector::operator=( const SparseUnsortedVector &other )
{
    _size = other._size;
    _vector = other._vector;
    _committedChanges = other._committedChanges;

    return *this;
}

void SparseUnsortedVector::storeIntoOther( SparseUnsortedVector *other ) const
{
    other->_size = _size;
    other->_vector = _vector;
    other->_committedChanges = _committedChanges;
}

HashMap<unsigned, double>::const_iterator SparseUnsortedVector::begin() const
{
    return _vector.begin();
}

HashMap<unsigned, double>::const_iterator SparseUnsortedVector::end() const
{
    return _vector.end();
}

void SparseUnsortedVector::commitChange( unsigned index, double newValue )
{
    _committedChanges[index] = newValue;
}

void SparseUnsortedVector::executeChanges()
{
    if ( _committedChanges.empty() )
        return;

    for ( const auto &change : _committedChanges )
    {
        unsigned index = change.first;
        double value = change.second;

        if ( !FloatUtils::isZero( value ) )
        {
            _vector.set( index, value );
        }
        else
        {
            if ( _vector.exists( index ) )
                _vector.erase( index );
        }
    }

    _committedChanges.clear();
}

void SparseUnsortedVector::set( unsigned index, double value )
{
    if ( !FloatUtils::isZero( value ) )
        _vector[index] = value;
    else if ( _vector.exists( value ) )
        _vector.erase( value );
}

void SparseUnsortedVector::addLastEntry( double entry )
{
    if ( !FloatUtils::isZero( entry ) )
        _vector[_size] = entry;

    ++_size;
}

void SparseUnsortedVector::incrementSize()
{
    ++_size;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
