
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
class SmtCore;

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
    void initializeFromTableau( const ITableau& tableau );
    void setStatistics( Statistics* statistics );
    List<Pair<unsigned, unsigned> > getConstraintsAndSplitsCausingFacts(List<const Fact*> facts) const;
    void addBoundFact( unsigned var, Tightening bound );
    void addEquationFact ( unsigned equNumber, Equation equ );
    bool hasFact(const Fact* fact) const;
    bool hasFactAffectingBound( unsigned var, BoundType type ) const;
    bool hasFactAffectingEquation( unsigned equNumber ) const;
    const Fact* getFactAffectingBound( unsigned var, BoundType type ) const;
    const Fact* getFactAffectingEquation( unsigned equNumber ) const;
    unsigned getNumFacts( ) const;
    Set<const Fact*> getExternalFactsForBound( const Fact* fact ) const;
    void popFact( );
    void verifySplitLevel( unsigned level, Set<unsigned> constraints ) const;
    SmtCore* _smtCore;

private:
    // Guy: names should be more informative, e.g. varToLowerBoundFact, like you did
    // with factFromIndex
    // Also, a conceptual issue: what if we learn x>=5 and later we learn x>=7? Are these always
    // the tightest bounds? What happens if later x>=5 is used for a deduction?
    // We should write an informative comment
    Map<unsigned, Stack<const Fact*> > _lowerBoundFact;
    Map<unsigned, Stack<const Fact*> > _upperBoundFact;
    Map<unsigned, Stack<const Fact*> > _equationFact;

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
