/*********************                                                        */
/*! \file ConstraintViolationTracker.h
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

#ifndef __ConstraintViolationTracker_h__
#define __ConstraintViolationTracker_h__

#include "List.h"
#include "MStringf.h"
#include "PLConstraintScoreTracker.h"
#include "Statistics.h"

class ConstraintViolationTracker : public PLConstraintScoreTracker
{
public:
    ConstraintViolationTracker();

    /*
      Increment the number of violations.
    */
    virtual void updateScore( PiecewiseLinearConstraint *constraint,
                              double score ) override;
};

#endif // __ConstraintViolationTracker_h__
