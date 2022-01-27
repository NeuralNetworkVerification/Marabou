/*********************                                                        */
/*! \file PLConstraintScoreTracker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A general class that maintains a heap from PLConstraint to a score.

**/

#ifndef __PLConstraintScoreTracker_h__
#define __PLConstraintScoreTracker_h__

#include "List.h"
#include "MStringf.h"
#include "PiecewiseLinearConstraint.h"

#include <set>

#define SCORE_TRACKER_LOG(x, ...) LOG(GlobalConfiguration::SCORE_TRACKER_LOGGING, "PLConstraintScoreTracker: %s\n", x)

struct ScoreEntry
{
    ScoreEntry( PiecewiseLinearConstraint *constraint, double score )
        : _constraint( constraint )
        , _score( score )
    {};

    bool operator<(const ScoreEntry& other ) const
    {
        if ( _score == other._score )
            return _constraint > other._constraint;
        else
            return _score > other._score;
    }

    PiecewiseLinearConstraint *_constraint;
    double _score;
};

typedef std::set<ScoreEntry> Scores;

class PLConstraintScoreTracker
{
public:
    PLConstraintScoreTracker();

    void initialize( List<PiecewiseLinearConstraint *> &plConstraints );

    void reset();

    void updateScore( PiecewiseLinearConstraint *constraint, double score );

    PiecewiseLinearConstraint *topUnfixed()
    {
        for ( const auto &entry : _scores )
        {
            if ( entry._constraint->isActive() && !entry._constraint->phaseFixed()
                 && _candidatePlConstraints.exists( entry._constraint ) )
            {
                COST_TRACKER_LOG( Stringf( "Score of top unfixed plConstraint: %.2f",
                                           entry._score ).ascii() );
                return entry._constraint;
            }
        }
        ASSERT( false );
        return NULL;
    }

    List<PiecewiseLinearConstraint *> _candidatePlConstraints;

private:
    Scores _scores;
    Map<PiecewiseLinearConstraint *, double> _plConstraintToScore;
};

#endif // __PLConstraintScoreTracker_h__
