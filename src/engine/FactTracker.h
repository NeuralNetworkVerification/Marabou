
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

#include "Fact.h"
#include "Equation.h"
#include "Tightening.h"

class FactTracker
{
public:
  enum BoundType {
		LB = 0,
		UB = 1,
    };
  void addBoundFact( unsigned var, Tightening bound );
  void addEquationFact ( unsigned equNumber, Equation equ );
  bool hasFactAffectingBound( unsigned var, BoundType type) const;
  bool hasFactAffectingEquation( unsigned equNumber ) const;
  unsigned getFactIDAffectingBound( unsigned var, BoundType type) const;
  unsigned getFactIDAffectingEquation( unsigned equNumber ) const;
private:
  unsigned _numFacts;
  Map<unsigned, unsigned> _lowerBoundFact;
  Map<unsigned, unsigned> _upperBoundFact;
  Map<unsigned, unsigned> _equationFact;
  Map<unsigned, Fact> _factFromIndex;
};

#endif // __FactTracker_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
