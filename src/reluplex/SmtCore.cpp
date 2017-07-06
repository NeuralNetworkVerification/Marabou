/*********************                                                        */
/*! \file SmtCore.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "SmtCore.h"

SmtCore::SmtCore( IEngine *engine )
    : _engine( engine )
    , _needToSplit( false )
{
}

void SmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= SPLIT_THRESHOLD )
    {
        _needToSplit = true;
        _constraintForSplitting = constraint;
    }
}

bool SmtCore::needToSplit() const
{
    return _needToSplit;
}

void SmtCore::performSplit()
{
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
