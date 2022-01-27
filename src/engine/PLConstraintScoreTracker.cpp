/*********************                                                        */
/*! \file PLConstraintScoreTracker.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "PLConstraintScoreTracker.h"

void PLConstraintScoreTracker::reset()
{
    _scores.clear();
    _plConstraintToScore.clear();
}

void PLConstraintScoreTracker::initialize( List<PiecewiseLinearConstraint *>
                                           &plConstraints )
{
    reset();
    for ( const auto &constraint : plConstraints )
    {
        _scores.insert( { constraint, 0 } );
        _plConstraintToScore[constraint] = 0;
    }
}

PiecewiseLinearConstraint *PLConstraintScoreTracker::topUnfixed()
{
    for ( const auto &entry : _scores )
    {
        if ( entry._constraint->isActive() && !entry._constraint->phaseFixed() )
        {
            SCORE_TRACKER_LOG( Stringf( "Score of top unfixed plConstraint: %.2f",
                                        entry._score ).ascii() );
            return entry._constraint;
        }
    }
    ASSERT( false );
    return NULL;
}
