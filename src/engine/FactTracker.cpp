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
#include "assert.h"

void FactTracker::addBoundFact( unsigned var, Tightening bound )
{
    unsigned newFactNum = _factsLearned.size();
    _factFromIndex[newFactNum] = bound;
    if ( bound._type == Tightening::LB )
    {
        if ( !hasFactAffectingBound( var, FactTracker::LB) )
          _lowerBoundFact[var] = Stack<unsigned>();
        _lowerBoundFact[var].push( newFactNum );
        _factsLearned.push( Pair<unsigned, BoundType>( var, FactTracker::LB ) );
    }
    else
    {
        if ( !hasFactAffectingBound( var, FactTracker::UB) )
          _upperBoundFact[var] = Stack<unsigned>();
        _upperBoundFact[var].push( newFactNum );
        _factsLearned.push( Pair<unsigned, BoundType>( var, FactTracker::UB ) );
    }
}

void FactTracker::addEquationFact( unsigned equNumber, Equation equ )
{
    unsigned newFactNum = _factsLearned.size();
    _factFromIndex[newFactNum] = equ;
    if ( !hasFactAffectingEquation( equNumber ) )
      _equationFact[equNumber] = Stack<unsigned>();
    _equationFact[equNumber].push(newFactNum);
    _factsLearned.push( Pair<unsigned, BoundType>( equNumber, FactTracker::EQU ) );
}

bool FactTracker::hasFactAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
        return _lowerBoundFact.exists( var ) && !_lowerBoundFact[var].empty();
    else
        return _upperBoundFact.exists( var ) && !_upperBoundFact[var].empty();
}

unsigned FactTracker::getFactIDAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
    {
        Stack<unsigned> temp = _lowerBoundFact[var];
        return temp.top();
    }
    else
    {
        Stack<unsigned> temp = _upperBoundFact[var];
        return temp.top();
    }
}

bool FactTracker::hasFactAffectingEquation( unsigned equNumber ) const
{
    return _equationFact.exists( equNumber ) && !_equationFact[equNumber].empty();
}

unsigned FactTracker::getFactIDAffectingEquation( unsigned equNumber ) const
{
    Stack<unsigned> temp = _equationFact[equNumber];
    return temp.top();
}

unsigned FactTracker::getNumFacts( ) const
{
  return _factsLearned.size();
}

void FactTracker::popFact()
{
  Pair<unsigned, BoundType> factInfo = _factsLearned.top();
  _factsLearned.pop();
  if ( factInfo.second() == LB )
    _lowerBoundFact[factInfo.first()].pop();
  if ( factInfo.second() == UB )
    _upperBoundFact[factInfo.first()].pop();
  if ( factInfo.second() == EQU )
    _equationFact[factInfo.first()].pop();
}

Set<unsigned> FactTracker::getExternalFactsForBound( unsigned explanationID ) const
{
    Set<unsigned> externalFacts;
    Fact fact = _factFromIndex[explanationID];
    List<unsigned> explanations = fact.getExplanations();
    List<bool> explanationIsInternal = fact.getExplanationIsInternal();
    List<unsigned>::iterator explanationsIter = explanations.begin();
    List<bool>::iterator explanationIsInternalIter = explanationIsInternal.begin();

    for ( ; explanationsIter != explanations.end() ; ++explanationsIter, ++explanationIsInternalIter )
    {
        if( !*explanationIsInternalIter )
        {
            externalFacts.insert( *explanationsIter );
        }
        else
        {
            externalFacts.insert( getExternalFactsForBound( *explanationsIter ) );
        }
    }

    return externalFacts;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
