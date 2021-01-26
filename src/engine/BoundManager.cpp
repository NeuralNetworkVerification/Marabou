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

void BoundManager::initialize( unsigned numberOfVariables)
{
    ASSERT( 0 == _size);

    _size = numberOfVariables;
    for ( unsigned i = 0; i < _size; ++i)
    {
        _lowerBounds.append( new (true) CDList<double>( &_context ) );
        _upperBounds.append( new (true) CDList<double>( &_context ) );
    }
}

bool BoundManager::updateLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _size );
    if ( value < getLowerBound( variable ) )
    {
        _lowerBounds[variable]->push_back( value );
        return true;
    }
    return false;
}

bool BoundManager::updateUpperBound( unsigned variable, double value )
{
     ASSERT( variable < _size );
    if ( value > getUpperBound( variable ) )
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
