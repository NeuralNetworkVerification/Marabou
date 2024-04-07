/*********************                                                        */
/*! \file PseudoImpactTracker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __PseudoImpactTracker_h__
#define __PseudoImpactTracker_h__

#include "List.h"
#include "MStringf.h"
#include "PLConstraintScoreTracker.h"
#include "Statistics.h"

class PseudoImpactTracker : public PLConstraintScoreTracker
{
public:
    PseudoImpactTracker();

    /*
      New score is the moving average of the input score and the previous score.
    */
    virtual void updateScore( PiecewiseLinearConstraint *constraint, double score ) override;
};

#endif // __PseudoImpactTracker_h__
