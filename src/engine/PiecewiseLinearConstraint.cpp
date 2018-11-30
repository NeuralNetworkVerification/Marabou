/*********************                                                        */
/*! \file PiecewiseLinearConstraint.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "PiecewiseLinearConstraint.h"
#include "FactTracker.h"
#include "Statistics.h"

PiecewiseLinearConstraint::PiecewiseLinearConstraint()
    : _constraintActive( true )
    , _statistics( NULL )
{
}

unsigned PiecewiseLinearConstraint::getID() const {
  return _id;
}

void PiecewiseLinearConstraint::setFactTracker( FactTracker* factTracker)
{
  _factTracker = factTracker;
}

void PiecewiseLinearConstraint::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
