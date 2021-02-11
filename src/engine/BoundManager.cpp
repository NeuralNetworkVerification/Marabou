/*********************                                                        */
/*! \file BoundManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "BoundManager.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Tableau.h"
#include "Tightening.h"

using namespace CVC4::context;

BoundManager::BoundManager( Context &context )
    : _context( context )
    , _size( 0 )
    , _tableau( nullptr )
{
};

BoundManager::~BoundManager()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        _lowerBounds[i]->deleteSelf();
        _upperBounds[i]->deleteSelf();
        _tightenedLower[i]->deleteSelf();
        _tightenedUpper[i]->deleteSelf();
    }
};

void BoundManager::initialize( unsigned numberOfVariables )
{
    ASSERT( _size == 0 );

    for ( unsigned i = 0; i < numberOfVariables; ++i )
        registerNewVariable();

    ASSERT( _size == numberOfVariables );
}

unsigned BoundManager::registerNewVariable()
{
    ASSERT( _size == _lowerBounds.size() );
    ASSERT( _size == _upperBounds.size() );
    ASSERT( _size == _tightenedLower.size() );
    ASSERT( _size == _tightenedUpper.size() );

    unsigned newVar = _size++;

    _lowerBounds.append( new ( true ) CDO<double>( &_context ) );
    _upperBounds.append( new ( true ) CDO<double>( &_context ) );
    _tightenedLower.append( new ( true ) CDO<bool>( &_context ) );
    _tightenedUpper.append( new ( true ) CDO<bool>( &_context ) );

    *_lowerBounds[newVar] = FloatUtils::negativeInfinity();
    *_upperBounds[newVar] = FloatUtils::infinity();
    *_tightenedLower[newVar] = false;
    *_tightenedUpper[newVar] = false;

    return newVar;
}

unsigned BoundManager::getNumberOfVariables() const
{
    return _size;
}

bool BoundManager::tightenLowerBound( unsigned variable, double value )
{
    bool tightened = setLowerBound( variable, value );
    if ( tightened && _tableau != nullptr )
        _tableau->ensureNonBasicVariableGTLB( variable, value );
    return tightened;
}

bool BoundManager::tightenUpperBound( unsigned variable, double value )
{
    bool tightened = setUpperBound( variable, value );
    if ( tightened && _tableau != nullptr )
        _tableau->ensureNonBasicVariableLTUB( variable, value );
    return tightened;
}

bool BoundManager::setLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value > getLowerBound( variable ) )
    {
        *_lowerBounds[variable] = value;
        *_tightenedLower[variable] = true;
        if ( !consistentBounds( variable ) )
            throw InfeasibleQueryException();
        return true;
    }
    return false;
}

bool BoundManager::setUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value < getUpperBound( variable ) )
    {
        *_upperBounds[variable] = value;
        *_tightenedUpper[variable] = true;
        if ( !consistentBounds( variable ) )
            throw InfeasibleQueryException();
        return true;
    }
    return false;
}

double BoundManager::getLowerBound( unsigned variable )
{
    ASSERT( variable < _size );
    return *_lowerBounds[variable];
}

double BoundManager::getUpperBound( unsigned variable )
{
    ASSERT( variable < _size );
    return *_upperBounds[variable];
}

void BoundManager::getTightenings( List<Tightening> &tightenings )
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( *_tightenedLower[i] )
        {
            tightenings.append( Tightening( i, *_lowerBounds[i], Tightening::LB ) );
            *_tightenedLower[i] = false;
        }

        if ( *_tightenedUpper[i] )
        {
            tightenings.append( Tightening( i, *_upperBounds[i], Tightening::UB ) );
            *_tightenedUpper[i] = false;
        }
    }
}

bool BoundManager::consistentBounds( unsigned variable )
{
    ASSERT( variable < _size );
    return FloatUtils::gte( getUpperBound( variable ), getLowerBound( variable ) );
}

void BoundManager::registerTableauReference( Tableau *ptrTableau )
{
    ASSERT( _tableau == nullptr );
    _tableau = ptrTableau;
}
