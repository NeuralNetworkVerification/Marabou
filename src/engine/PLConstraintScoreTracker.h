/*********************                                                        */
/*! \file PLConstraintScoreTracker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A general class that maintains a heap from PLConstraint to a score.

**/

#ifndef __PLConstraintScoreTracker_h__
#define __PLConstraintScoreTracker_h__

#include "Debug.h"
#include "List.h"
#include "MStringf.h"
#include "PiecewiseLinearConstraint.h"

#include <set>

#define SCORE_TRACKER_LOG( x, ... )                                                                \
    LOG( GlobalConfiguration::SCORE_TRACKER_LOGGING, "PLConstraintScoreTracker: %s\n", x )

struct ScoreEntry
{
    ScoreEntry( PiecewiseLinearConstraint *constraint, double score )
        : _constraint( constraint )
        , _score( score ){};

    bool operator<( const ScoreEntry &other ) const
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
    virtual ~PLConstraintScoreTracker() = default;

    /*
      Initialize the scores for all constraints to 0.
    */
    void initialize( const List<PiecewiseLinearConstraint *> &plConstraints );

    /*
      Empty the local variables.
    */
    void reset();

    /*
      Update the score of a constraint.
    */
    virtual void updateScore( PiecewiseLinearConstraint *constraint, double score ) = 0;

    /*
      Set the score of a constraint.
    */
    void setScore( PiecewiseLinearConstraint *constraint, double score );

    /*
      Among active and unfixed constraints, return the one with the largest
      score.
    */
    PiecewiseLinearConstraint *topUnfixed();

    /*
      Return the constraint with the largest score.
    */
    inline PiecewiseLinearConstraint *top()
    {
        return _scores.begin()->_constraint;
    }

    /*
      Get the score of the PLConstraint
    */
    inline double getScore( PiecewiseLinearConstraint *constraint )
    {
        DEBUG( {
            ASSERT( _plConstraintToScore.exists( constraint ) );
            ASSERT( _scores.find( ScoreEntry( constraint, _plConstraintToScore[constraint] ) ) !=
                    _scores.end() );
        } );
        return _plConstraintToScore[constraint];
    }

protected:
    Scores _scores;
    Map<PiecewiseLinearConstraint *, double> _plConstraintToScore;
};

#endif // __PLConstraintScoreTracker_h__
