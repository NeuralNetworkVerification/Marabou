
/*********************                                                        */
/*! \file Fact.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __FactTracker_h__
#define __FactTracker_h__

#include "Equation.h"
#include "Fact.h"
#include "Pair.h"
#include "Stack.h"
#include "Statistics.h"
#include "Tightening.h"

class ITableau;

class FactTracker
{
public:
    enum BoundType {
		LB = 0,
		UB = 1,
    EQU = 2,
    };

    FactTracker(): _statistics(NULL){};
    ~FactTracker();

    /*
      Get all facts currently in tableau
    */
    void initializeFromTableau( const ITableau& tableau );

    void setStatistics( Statistics* statistics );

    /*
      Return list of constraint/split IDs that led to us learning a set of facts
    */
    List<Pair<unsigned, unsigned> > getConstraintsAndSplitsCausingFacts(List<const Fact*> facts) const;

    /*
      Add facts learned about variable bounds or equations
    */
    void addBoundFact( unsigned var, Tightening bound );
    void addEquationFact ( unsigned equNumber, Equation equ );

    /*
      Check whether we know a fact about a variable bound or equation
    */
    bool hasFact(const Fact* fact) const;
    bool hasFactAffectingBound( unsigned var, BoundType type ) const;
    bool hasFactAffectingEquation( unsigned equNumber ) const;

    /*
      Get most recent fact affecting bounds or equations
    */
    const Fact* getFactAffectingBound( unsigned var, BoundType type ) const;
    const Fact* getFactAffectingEquation( unsigned equNumber ) const;

    /*
      Get number of facts currently tracked
    */
    unsigned getNumFacts( ) const;

    /*
      Get set of facts that explain a given fact but are not tracked by this object
    */
    Set<const Fact*> getExternalFactsForBound( const Fact* fact ) const;

    /*
      Forget a learned fact
    */
    void popFact( );

private:

    /*
      Stacks of facts learned about upper and lower bounds for each Variables
      And equations learned
    */
    Map<unsigned, Stack<const Fact*> > _lowerBoundFact;
    Map<unsigned, Stack<const Fact*> > _upperBoundFact;
    Map<unsigned, Stack<const Fact*> > _equationFact;

    /*
      Set of all facts learned, and stack of information about what the facts describe
    */
    Set<const Fact*> _factsLearnedSet;
    Stack<Pair<unsigned, BoundType> > _factsLearned;

    Statistics* _statistics;
};

#endif // __FactTracker_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
