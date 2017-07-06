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

#include "IEngine.h"
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
    // Obtain the splits
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();

    // Perform the first split: add bounds and equations
    List<PiecewiseLinearCaseSplit>::iterator split = splits.begin();
    _engine->addNewEquation( split->getEquation() );
    List<PiecewiseLinearCaseSplit::Bound> bounds = split->getBoundTightenings();
    for ( const auto &bound : bounds )
    {
        if ( bound._boundType == PiecewiseLinearCaseSplit::Bound::LOWER )
            _engine->tightenLowerBound( bound._variable, bound._newBound );
        else
            _engine->tightenUpperBound( bound._variable, bound._newBound );
    }

    // Store the remaining splits for later
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
