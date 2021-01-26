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

#include "FloatUtils.h"
#include "BoundManager.h"
#include "Debug.h"

using namespace CVC4::context;

BoundManager::BoundManager( Context &context)
    : _context(context)
    , _size(0)
{
};

BoundManager::~BoundManager()
{
    for ( unsigned i = 0; i < _size; ++i)
    {
        _lowerBounds[i]->deleteSelf();
        _upperBounds[i]->deleteSelf();
    }
};

unsigned BoundManager::registerNewVariable()
{
    ASSERT( _lowerBounds.size() == _size );
    ASSERT( _upperBounds.size() == _size );

    unsigned newVar = _size++; 

    _lowerBounds.append( new (true) CDList<double>( &_context ) );
    _upperBounds.append( new (true) CDList<double>( &_context ) );

    _lowerBounds[newVar]->push_back( FloatUtils::negativeInfinity() );
    _upperBounds[newVar]->push_back( FloatUtils::infinity() );

    ASSERT( _lowerBounds.size() == _size );
    ASSERT( _upperBounds.size() == _size );

    return newVar;
}

void BoundManager::initialize( unsigned numberOfVariables )
{
    ASSERT( 0 == _size);

    for ( unsigned i = 0; i < numberOfVariables; ++i)
        registerNewVariable();

    ASSERT( numberOfVariables == _size );
}

bool BoundManager::setLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value > getLowerBound( variable ) )
    {
        _lowerBounds[variable]->push_back( value );
        return true;
    }
    return false;
}

bool BoundManager::setUpperBound( unsigned variable, double value )
{
     ASSERT( variable < _size );
    if ( value < getUpperBound( variable ) )
    {
        _upperBounds[variable]->push_back( value );
        return true;
    }
    return false;
}

double BoundManager::getLowerBound( unsigned variable )
{
  ASSERT( variable < _size );
  return _lowerBounds[variable]->back();
}


double BoundManager::getUpperBound( unsigned variable )
{
  ASSERT( variable < _size );
  return _upperBounds[variable]->back();
}

unsigned BoundManager::getSize( )
{
  return _size;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
