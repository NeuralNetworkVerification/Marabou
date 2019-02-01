/*********************                                                        */
/*! \file RowBoundTightener.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "ConstraintBoundTightener.h"
#include "FloatUtils.h"
#include "ReluplexError.h"
#include "Statistics.h"
#include "FactTracker.h"

ConstraintBoundTightener::ConstraintBoundTightener( const ITableau &tableau )
    : _tableau( tableau )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _tightenedLower( NULL )
    , _tightenedUpper( NULL )
    , _lowerBoundsExplanation( NULL )
    , _upperBoundsExplanation( NULL )
    , _factTracker( NULL )
    , _statistics( NULL )
{
}

void ConstraintBoundTightener::setDimensions()
{
    freeMemoryIfNeeded();

    _n = _tableau.getN();
    _m = _tableau.getM();

    _lowerBounds = new double[_n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::lowerBounds" );

    _upperBounds = new double[_n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::upperBounds" );

    _lowerBoundsExplanation = new unsigned[_n];
    if ( !_lowerBoundsExplanation )
      throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::lowerBoundsExplanation" );
    
    _upperBoundsExplanation = new unsigned[_n];
    if ( !_upperBoundsExplanation )
      throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::upperBoundsExplanation" );

    _tightenedLower = new bool[_n];
    if ( !_tightenedLower )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::tightenedLower" );

    _tightenedUpper = new bool[_n];
    if ( !_tightenedUpper )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ConstraintBoundTightener::tightenedUpper" );

    resetBounds();
}

void ConstraintBoundTightener::resetBounds()
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = _tableau.getLowerBound( i );
        _upperBounds[i] = _tableau.getUpperBound( i );

        if( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::UB ))
        {
           _upperBoundsExplanation[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::UB );
        }

        if( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::LB ))
        {
           _lowerBoundsExplanation[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::LB );
        }
    }
}

ConstraintBoundTightener::~ConstraintBoundTightener()
{
    freeMemoryIfNeeded();
}

void ConstraintBoundTightener::freeMemoryIfNeeded()
{
    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _lowerBoundsExplanation )
    {
        delete[] _lowerBoundsExplanation;
        _lowerBoundsExplanation = NULL;
    }

    if ( _upperBoundsExplanation )
    {
        delete[] _upperBoundsExplanation;
        _upperBoundsExplanation = NULL;
    }

    if ( _tightenedLower )
    {
        delete[] _tightenedLower;
        _tightenedLower = NULL;
    }

    if ( _tightenedUpper )
    {
        delete[] _tightenedUpper;
        _tightenedUpper = NULL;
    }
}

void ConstraintBoundTightener::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void ConstraintBoundTightener::setFactTracker( FactTracker *factTracker )
{
    _factTracker = factTracker;
}

void ConstraintBoundTightener::notifyLowerBound( unsigned variable, double bound )
{
    if ( bound > _lowerBounds[variable] )
    {
        _lowerBounds[variable] = bound;
        _tightenedLower[variable] = false;

        if( _factTracker && _factTracker->hasFactAffectingBound( variable, FactTracker::LB ))
        {
           _lowerBoundsExplanation[variable] = _factTracker->getFactIDAffectingBound( variable, FactTracker::LB );
        }
    }
}

void ConstraintBoundTightener::notifyUpperBound( unsigned variable, double bound )
{
    if ( bound < _upperBounds[variable] )
    {
        _upperBounds[variable] = bound;
        _tightenedUpper[variable] = false;

        if( _factTracker && _factTracker->hasFactAffectingBound( variable, FactTracker::UB ))
        {
           _upperBoundsExplanation[variable] = _factTracker->getFactIDAffectingBound( variable, FactTracker::UB );
        }
    }
}

void ConstraintBoundTightener::notifyDimensionChange( unsigned /* m */ , unsigned /* n */ )
{
    setDimensions();
}

void ConstraintBoundTightener::registerTighterLowerBound( unsigned variable, double bound, unsigned explanationID )
{
    if ( bound > _lowerBounds[variable] )
    {
        _lowerBounds[variable] = bound;
        _tightenedLower[variable] = true;
        _lowerBoundsExplanation[variable] = explanationID;
    }
}

void ConstraintBoundTightener::registerTighterUpperBound( unsigned variable, double bound, unsigned explanationID )
{
    if ( bound < _upperBounds[variable] )
    {
        _upperBounds[variable] = bound;
        _tightenedUpper[variable] = true;
        _upperBoundsExplanation[variable] = explanationID;
    }
}

void ConstraintBoundTightener::getConstraintTightenings( List<Tightening> &tightenings ) const
{
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( _tightenedLower[i] )
        {
            Tightening lowerBoundTightening( i, _lowerBounds[i], Tightening::LB );
            lowerBoundTightening.addExplanation( _lowerBoundsExplanation[i] );
            tightenings.append( lowerBoundTightening );
            // Junyao: why not _tightenedLower[i] = false; as in RowBoundTightener
        }

        if ( _tightenedUpper[i] )
        {
            Tightening upperBoundTightening( i, _upperBounds[i], Tightening::UB );
            upperBoundTightening.addExplanation( _upperBoundsExplanation[i] );
            tightenings.append( upperBoundTightening );
            // Junyao: why not _tightenedUpper[i] = false; as in RowBoundTightener
        }
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
