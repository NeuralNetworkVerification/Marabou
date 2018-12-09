/*********************                                                        */
/*! \file Fact.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "FactTracker.h"

void FactTracker::addBoundFact( unsigned var, Tightening bound )
{
    _factFromIndex[_numFacts] = bound;
    if ( bound._type == Tightening::LB )
        _lowerBoundFact[var] = _numFacts;
    else
        _upperBoundFact[var] = _numFacts;
    ++_numFacts;
}

void FactTracker::addEquationFact( unsigned equNumber, Equation equ )
{
    _factFromIndex[_numFacts] = equ;
    _equationFact[equNumber] = _numFacts;
    ++_numFacts;
}

bool FactTracker::hasFactAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
        return _lowerBoundFact.exists( var );
    else
        return _upperBoundFact.exists( var );
}

unsigned FactTracker::getFactIDAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
        return _lowerBoundFact[var];
    else
        return _upperBoundFact[var];
}

bool FactTracker::hasFactAffectingEquation( unsigned equNumber ) const
{
    return _equationFact.exists( equNumber );
}

unsigned FactTracker::getFactIDAffectingEquation( unsigned equNumber ) const
{
    return _equationFact[equNumber];
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
