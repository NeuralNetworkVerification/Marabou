/*********************                                                        */
/*! \file NonlinearConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in NonlinearConstraint.h.
**/

#include "NonlinearConstraint.h"
#include "Statistics.h"

NonlinearConstraint::NonlinearConstraint()
    : _boundManager( nullptr )
    , _constraintBoundTightener( NULL )
    , _statistics( NULL )
{
}

void NonlinearConstraint::registerBoundManager(
    BoundManager *boundManager )
{
    ASSERT( _boundManager == nullptr );
    _boundManager = boundManager;
}

void NonlinearConstraint::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void NonlinearConstraint::registerConstraintBoundTightener( IConstraintBoundTightener *tightener )
{
    _constraintBoundTightener = tightener;
}
