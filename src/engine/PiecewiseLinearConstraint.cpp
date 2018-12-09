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

#include "FactTracker.h"
#include "PiecewiseLinearConstraint.h"
#include "Statistics.h"

PiecewiseLinearConstraint::PiecewiseLinearConstraint()
    : _constraintActive( true )
    , _id( 0 )
    , _constraintBoundTightener( NULL )
      // Guy: since _factTracker is a member of the parent class, better to initialize it here
      // instead of in the children
    , _factTracker( NULL )
    , _statistics( NULL )
{
}

PiecewiseLinearConstraint::PiecewiseLinearConstraint( unsigned id )
    : _constraintActive( true )
    , _id( id )
    , _constraintBoundTightener( NULL )
    , _factTracker( NULL )
    , _statistics( NULL )
{
}

unsigned PiecewiseLinearConstraint::getID() const
{
    return _id;
}

void PiecewiseLinearConstraint::setFactTracker( FactTracker *factTracker )
{
    _factTracker = factTracker;
}

void PiecewiseLinearConstraint::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void PiecewiseLinearConstraint::registerConstraintBoundTightener( IConstraintBoundTightener *tightener )
{
    _constraintBoundTightener = tightener;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
