/*********************                                                        */
/*! \file PiecewiseLinearConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "PiecewiseLinearConstraint.h"
#include "Statistics.h"

PiecewiseLinearConstraint::PiecewiseLinearConstraint()
    : _constraintActive( true )
    , _violatedPlConstraints( NULL )
    , _constraintToViolationCount( NULL )
    , _violationPriorityQueue( NULL )
    , _constraintBoundTightener( NULL )
    , _statistics( NULL )
{
}

void PiecewiseLinearConstraint::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void PiecewiseLinearConstraint::registerConstraintBoundTightener( IConstraintBoundTightener *tightener )
{
    _constraintBoundTightener = tightener;
}
void PiecewiseLinearConstraint::registerViolationWatchers(
  Set<PiecewiseLinearConstraint*>* violatedPlConstraints,
  Map<PiecewiseLinearConstraint *, unsigned>* constraintToViolationCount,
  Set<Pair<unsigned, PiecewiseLinearConstraint*> >* violationPriorityQueue)
{
  _violatedPlConstraints = violatedPlConstraints;
  _constraintToViolationCount = constraintToViolationCount;
  _violationPriorityQueue = violationPriorityQueue;
  if ( shouldBeInViolationSet() ){
    _violatedPlConstraints->insert( this );
    // TODO
  }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
