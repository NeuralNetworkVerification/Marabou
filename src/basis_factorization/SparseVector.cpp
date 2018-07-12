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

#include "Debug.h"
#include "SparseVector.h"

SparseVector::SparseVector()
{
}

SparseVector::SparseVector( unsigned size )
{
    _V.initializeToEmpty( 1, size );
}

SparseVector::SparseVector( const double *V, unsigned size )
{
    initialize( V, size );
}

void SparseVector::initialize( const double *V, unsigned size )
{
    _V.initialize( V, 1, size );
}

void SparseVector::initializeToEmpty( unsigned size )
{
    _V.initializeToEmpty( 1, size );
}

void SparseVector::clear()
{
    _V.clear();
}

unsigned SparseVector::getNnz() const
{
    return _V.getNnz();
}

bool SparseVector::empty() const
{
    return getNnz() == 0;
}

double SparseVector::get( unsigned index )
{
    return _V.get( 0, index );
}

void SparseVector::dump() const
{
    _V.dump();
}

void SparseVector::toDense( double *result ) const
{
    _V.getRowDense( 0, result );
}

SparseVector &SparseVector::operator=( const SparseVector &other )
{
    other._V.storeIntoOther( &_V );
    return *this;
}

CSRMatrix *SparseVector::getInternalMatrix()
{
    return &_V;
}

unsigned SparseVector::getIndexOfEntry( unsigned entry ) const
{
    ASSERT( entry < getNnz() );
    return _V.getJA()[entry];
}

double SparseVector::getValueOfEntry( unsigned entry ) const
{
    ASSERT( entry < getNnz() );
    return _V.getA()[entry];
}

void SparseVector::addEmptyLastEntry()
{
    _V.addEmptyColumn();
}

void SparseVector::addLastEntry( double value )
{
    _V.addLastColumn( &value );
}

void SparseVector::commitChange( unsigned index, double newValue )
{
    _V.commitChange( 0, index, newValue );
}

void SparseVector::executeChanges()
{
    _V.executeChanges();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
