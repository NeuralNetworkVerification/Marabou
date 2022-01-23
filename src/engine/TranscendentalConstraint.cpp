/*********************                                                        */
/*! \file TranscendentalConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in TranscendentalConstraint.h.
**/

#include "TranscendentalConstraint.h"
#include "Statistics.h"

TranscendentalConstraint::TranscendentalConstraint()
    : _boundManager( nullptr )
    , _constraintBoundTightener( NULL )
    , _statistics( NULL )
{
}

void TranscendentalConstraint::registerBoundManager(
    BoundManager *boundManager )
{
    ASSERT( _boundManager == nullptr );
    _boundManager = boundManager;
}

void TranscendentalConstraint::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void TranscendentalConstraint::registerConstraintBoundTightener( IConstraintBoundTightener *tightener )
{
    _constraintBoundTightener = tightener;
}
