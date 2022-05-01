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
#include "MarabouError.h"

using namespace CVC4::context;

BoundManager::BoundManager( Context &context )
    : _context( context )
    , _size( 0 )
    , _allocated( 0 )
    , _tableau( nullptr )
    , _rowBoundTightener( nullptr )
    , _consistentBounds( &_context )
    , _firstInconsistentTightening( 0, 0.0, Tightening::LB )
    , _lowerBounds( nullptr )
    , _upperBounds( nullptr )
{
    _consistentBounds = true;
};

BoundManager::~BoundManager()
{
    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = nullptr;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = nullptr;
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        _storedLowerBounds[i]->deleteSelf();
        _storedUpperBounds[i]->deleteSelf();
        _tightenedLower[i]->deleteSelf();
        _tightenedUpper[i]->deleteSelf();
    }
};

void BoundManager::initialize( unsigned numberOfVariables )
{
    ASSERT( _size == 0 );

    allocateLocalBounds( numberOfVariables );

    for ( unsigned i = 0; i < numberOfVariables; ++i )
        registerNewVariable();

    ASSERT( _size == numberOfVariables );
    ASSERT( _allocated == numberOfVariables );
}

void BoundManager::allocateLocalBounds( unsigned size )
{
    _lowerBounds = new double[size];
    if ( !_lowerBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "BoundManager::lowerBounds" );
    std::fill_n( _lowerBounds, size, FloatUtils::negativeInfinity() );

    _upperBounds = new double[size];
    if ( !_upperBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "BoundManager::upperBounds" );
    std::fill_n( _upperBounds, size, FloatUtils::infinity() );
    _allocated = size;

    if ( _tableau )
      _tableau->setBoundsPointers( _lowerBounds, _upperBounds );

    if ( _rowBoundTightener )
      _rowBoundTightener->setBoundsPointers( _lowerBounds, _upperBounds );
}

unsigned BoundManager::registerNewVariable()
{
    ASSERT( _size == _storedLowerBounds.size() );
    ASSERT( _size == _storedUpperBounds.size() );
    ASSERT( _size == _tightenedLower.size() );
    ASSERT( _size == _tightenedUpper.size() );

    unsigned newVar = _size++;

    if ( _allocated < _size )
    {
      double * oldLowerBounds = _upperBounds;
      double * oldUpperBounds = _lowerBounds;

      allocateLocalBounds( 2*_allocated );
      std::memcpy( _lowerBounds, oldLowerBounds, _allocated );
      std::memcpy( _upperBounds, oldUpperBounds, _allocated );
      _allocated *= 2;

      delete[] oldLowerBounds;
      delete[] oldUpperBounds;
    }

    _storedLowerBounds.append( new ( true ) CDO<double>( &_context ) );
    _storedUpperBounds.append( new ( true ) CDO<double>( &_context ) );
    _tightenedLower.append( new ( true ) CDO<bool>( &_context ) );
    _tightenedUpper.append( new ( true ) CDO<bool>( &_context ) );

    *_storedLowerBounds[newVar] = FloatUtils::negativeInfinity();
    *_storedUpperBounds[newVar] = FloatUtils::infinity();
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
        _tableau->updateVariableToComplyWithLowerBoundUpdate( variable, value );
    return tightened;
}

bool BoundManager::tightenUpperBound( unsigned variable, double value )
{
    bool tightened = setUpperBound( variable, value );
    if ( tightened && _tableau != nullptr )
        _tableau->updateVariableToComplyWithUpperBoundUpdate( variable, value );
    return tightened;
}

void BoundManager::recordInconsistentBound( unsigned variable, double value, Tightening::BoundType type )
{
  if ( _consistentBounds )
  {
    _consistentBounds = false;
    _firstInconsistentTightening = Tightening( variable, value, type );
  }
}

bool BoundManager::setLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value > _lowerBounds[variable] )
    {
        _lowerBounds[variable] = value;
        *_tightenedLower[variable] = true;
        if ( !consistentBounds( variable ) )
            recordInconsistentBound( variable, value, Tightening::LB );
        return true;
    }
    return false;
}


bool BoundManager::setUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value < _upperBounds[variable] )
    {
        _upperBounds[variable] = value;
        *_tightenedUpper[variable] = true;
        if ( !consistentBounds( variable ) )
          recordInconsistentBound( variable, value, Tightening::UB );
        return true;
    }
    return false;
}

double BoundManager::getLowerBound( unsigned variable ) const
{
    ASSERT( variable < _size );
    return _lowerBounds[variable];
}

double BoundManager::getUpperBound( unsigned variable ) const
{
    ASSERT( variable < _size );
    return _upperBounds[variable];
}

const double * BoundManager::getLowerBounds() const
{
    return _lowerBounds;
}

const double * BoundManager::getUpperBounds() const
{
    return _upperBounds;
}

void BoundManager::storeLocalBounds()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
      *_storedLowerBounds[i]=_lowerBounds[i];
      *_storedUpperBounds[i]=_upperBounds[i];
    }
}

void BoundManager::restoreLocalBounds()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
      _lowerBounds[i]=*_storedLowerBounds[i];
      _upperBounds[i]=*_storedUpperBounds[i];
    }
}

void BoundManager::getTightenings( List<Tightening> &tightenings )
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( *_tightenedLower[i] )
        {
            tightenings.append( Tightening( i, _lowerBounds[i], Tightening::LB ) );
            *_tightenedLower[i] = false;
        }

        if ( *_tightenedUpper[i] )
        {
            tightenings.append( Tightening( i, _upperBounds[i], Tightening::UB ) );
            *_tightenedUpper[i] = false;
        }
    }
}

void BoundManager::clearTightenings()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
      *_tightenedLower[i]=false;
      *_tightenedUpper[i]=false;
    }
}

void BoundManager::propagateTightenings()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
      if ( *_tightenedLower[i] )
      {
        _tableau->notifyLowerBound( i, getLowerBound( i ) );
        *_tightenedLower[i] = false;
      }

      if ( *_tightenedUpper[i] )
      {
        _tableau->notifyUpperBound( i, getUpperBound( i ) );
        *_tightenedUpper[i] = false;
      }
    }
}

bool BoundManager::consistentBounds() const
{
    return _consistentBounds;
}

bool BoundManager::consistentBounds( unsigned variable ) const
{
    ASSERT( variable < _size );
    return FloatUtils::gte( getUpperBound( variable ), getLowerBound( variable ) );
}

void BoundManager::registerTableau( ITableau *ptrTableau )
{
    ASSERT( _tableau == nullptr );
    _tableau = ptrTableau;
}

void BoundManager::registerRowBoundTightener( IRowBoundTightener *ptrRowBoundTightener )
{
    ASSERT( _rowBoundTightener == nullptr );
    _rowBoundTightener = ptrRowBoundTightener;
}
