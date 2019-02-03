
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

// Guy: sort includes alphabetically
#include "Equation.h"
#include "Fact.h"
#include "Pair.h"
#include "Stack.h"
#include "Statistics.h"
#include "Tightening.h"

class FactTracker
{
public:
    enum BoundType {
		LB = 0,
		UB = 1,
    EQU = 2,
    };

    FactTracker(): _statistics(NULL){};
    void setStatistics( Statistics* statistics );
    List<Pair<unsigned, unsigned> > getConstraintsAndSplitsCausingFacts(List<unsigned> facts) const;
    void addSplitLevelCausingFact( Fact* fact, unsigned factID );
    void addBoundFact( unsigned var, Tightening bound );
    void addEquationFact ( unsigned equNumber, Equation equ );
    bool hasFactAffectingBound( unsigned var, BoundType type ) const;
    bool hasFactAffectingEquation( unsigned equNumber ) const;
    unsigned getFactIDAffectingBound( unsigned var, BoundType type ) const;
    unsigned getFactIDAffectingEquation( unsigned equNumber ) const;
    unsigned getNumFacts( ) const;
    void popFact( );
    void printFactAndExplanations(unsigned factID) const;

private:
    unsigned _numFacts;

    // Guy: names should be more informative, e.g. varToLowerBoundFact, like you did
    // with factFromIndex
    // Also, a conceptual issue: what if we learn x>=5 and later we learn x>=7? Are these always
    // the tightest bounds? What happens if later x>=5 is used for a deduction?
    // We should write an informative comment
    Map<unsigned, Stack<unsigned> > _lowerBoundFact;
    Map<unsigned, Stack<unsigned> > _upperBoundFact;
    Map<unsigned, Stack<unsigned> > _equationFact;
    //

    Map<unsigned, Fact*> _factFromIndex;
    Stack<Pair<unsigned, BoundType> > _factsLearned;
    Map<unsigned, unsigned> _factToSplitLevelCausing;

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
