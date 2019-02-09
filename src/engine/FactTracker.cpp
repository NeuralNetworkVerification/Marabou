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
#include "Queue.h"

void FactTracker::setStatistics( Statistics* statistics )
{
  _statistics = statistics;
}

List<Pair<unsigned, unsigned> > FactTracker::getConstraintsAndSplitsCausingFacts(List<const Fact*> facts) const
{
  Set<const Fact*> seen;
  List<Pair<unsigned, unsigned> > result;
  Queue<const Fact*> remaining;
  for(const Fact* id: facts)
    remaining.push( id );
  while(!remaining.empty())
  {
    const Fact* fact = remaining.peak();
    remaining.pop();
    if (seen.exists(fact)) continue;
    seen.insert(fact);
    if(fact->isCausedBySplit()){
      result.append(Pair<unsigned, unsigned>(fact->getCausingConstraintID(), fact->getCausingSplitID()));
    }
    else {
      for(const Fact* explanation: fact->getExplanations())
        remaining.push(explanation);
    }
  }
  return result;
}


void FactTracker::addSplitLevelCausingFact( const Fact* fact)
{
  if ( fact->isCausedBySplit() )
  {
    _factToSplitLevelCausing[fact] = _statistics->getCurrentStackDepth();
    return;
  }
  unsigned level = 0;
  for (const Fact* explanation: fact->getExplanations() )
  {
    if (_factToSplitLevelCausing[explanation] > level)
      level = _factToSplitLevelCausing[explanation];
  }
  _factToSplitLevelCausing[fact] = level;
}

void FactTracker::addBoundFact( unsigned var, Tightening bound )
{
    const Fact* newFact = new Tightening(bound);
    _factsLearnedSet.insert(newFact);
    addSplitLevelCausingFact( newFact );
    if ( bound._type == Tightening::LB )
    {
        if ( !hasFactAffectingBound( var, FactTracker::LB) )
          _lowerBoundFact[var] = Stack<const Fact*>();
        _lowerBoundFact[var].push( newFact );
        _factsLearned.push( Pair<unsigned, BoundType>( var, FactTracker::LB ) );
    }
    else
    {
        if ( !hasFactAffectingBound( var, FactTracker::UB) )
          _upperBoundFact[var] = Stack<const Fact*>();
        _upperBoundFact[var].push( newFact );
        _factsLearned.push( Pair<unsigned, BoundType>( var, FactTracker::UB ) );
    }
}

void FactTracker::addEquationFact( unsigned equNumber, Equation equ )
{
    const Fact* newFact = new Equation(equ);
    _factsLearnedSet.insert(newFact);
    addSplitLevelCausingFact( newFact );
    if ( !hasFactAffectingEquation( equNumber ) )
      _equationFact[equNumber] = Stack<const Fact*>();
    _equationFact[equNumber].push(newFact);
    _factsLearned.push( Pair<unsigned, BoundType>( equNumber, FactTracker::EQU ) );
}

bool FactTracker::hasFact(const Fact* fact) const
{
  return _factsLearnedSet.exists(fact);
}

bool FactTracker::hasFactAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
        return _lowerBoundFact.exists( var ) && !_lowerBoundFact[var].empty();
    else
        return _upperBoundFact.exists( var ) && !_upperBoundFact[var].empty();
}

const Fact* FactTracker::getFactAffectingBound( unsigned var, BoundType type ) const
{
    if ( type == LB )
    {
        Stack<const Fact*> temp = _lowerBoundFact[var];
        return temp.top();
    }
    else
    {
        Stack<const Fact*> temp = _upperBoundFact[var];
        return temp.top();
    }
}

bool FactTracker::hasFactAffectingEquation( unsigned equNumber ) const
{
    return _equationFact.exists( equNumber ) && !_equationFact[equNumber].empty();
}

const Fact* FactTracker::getFactAffectingEquation( unsigned equNumber ) const
{
    Stack<const Fact*> temp = _equationFact[equNumber];
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
  if ( factInfo.second() == LB ){
    const Fact* oldFact = _lowerBoundFact[factInfo.first()].top();
    _factsLearnedSet.erase(oldFact);
    delete oldFact;
    _lowerBoundFact[factInfo.first()].pop();
  }
  if ( factInfo.second() == UB ){
    const Fact* oldFact = _upperBoundFact[factInfo.first()].top();
    _factsLearnedSet.erase(oldFact);
    delete oldFact;    _upperBoundFact[factInfo.first()].pop();
  }
  if ( factInfo.second() == EQU ){
    const Fact* oldFact = _equationFact[factInfo.first()].top();
    _factsLearnedSet.erase(oldFact);
    delete oldFact;
    _equationFact[factInfo.first()].pop();
  }
}

Set<const Fact*> FactTracker::getExternalFactsForBound( const Fact* fact ) const
{
    Set<const Fact*> externalFacts;
    List<const Fact*> explanations = fact->getExplanations();
    for(const Fact* explanation: explanations){
      if (!hasFact(explanation)){
        externalFacts.insert(explanation);
      }
      else{
        externalFacts.insert(getExternalFactsForBound(explanation));
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
