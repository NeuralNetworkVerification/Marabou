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

PLConstraintScoreTracker::PLConstraintScoreTracker()
{}

void PLConstraintScoreTracker::initialize( List<PiecewiseLinearConstraint *> &plConstraints )
{
    reset();
    for ( const auto &constraint : plConstraints )
    {
        _scores.insert( { constraint, 0 } );
        _plConstraintToScore[constraint] = 0;
    }
}

void PLConstraintScoreTracker::updateScore( PiecewiseLinearConstraint *constraint,
                                       double score )
{
    ASSERT( _plConstraintToScore.exists( constraint ) );

    double alpha = GlobalConfiguration::EXPONENTIAL_MOVING_AVERAGE_ALPHA;
    double oldScore = _plConstraintToScore[constraint];
    double newScore = ( 1 - alpha ) * oldScore  + alpha * score;

    ASSERT( _scores.find( ScoreEntry( constraint, oldScore ) ) != _scores.end() );
    _scores.erase( ScoreEntry( constraint, oldScore ) );
    _scores.insert( ScoreEntry( constraint, newScore ) );
    _plConstraintToScore[constraint] = newScore;
}

PiecewiseLinearConstraint *topUnfixed()
{
    for ( const auto &entry : _scores )
        if ( entry._constraint->isActive() && !entry._constraint->phaseFixed()
             && _candidatePlConstraints.exists( entry._constraint ) )
            {
                COST_TRACKER_LOG( Stringf( "Score of top unfixed plConstraint: %.2f",
                                           entry._score ).ascii() );
                return entry._constraint;
            }
    ASSERT( false );
    return NULL;
}
