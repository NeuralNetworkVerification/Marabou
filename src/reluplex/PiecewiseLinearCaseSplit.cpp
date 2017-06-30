/*********************                                                        */
/*! \file PiecewiseLinearCaseSplit.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "PiecewiseLinearCaseSplit.h"

void PiecewiseLinearCaseSplit::setBoundTightening( unsigned variable, bool upperBound, double newBound )
{
    _variable = variable;
    _upperBound = upperBound;
    _newBound = newBound;
}

unsigned PiecewiseLinearCaseSplit::getVariable() const
{
    return _variable;
}

bool PiecewiseLinearCaseSplit::getUpperBound() const
{
    return _upperBound;
}

double PiecewiseLinearCaseSplit::getNewBound() const
{
    return _newBound;
}

void PiecewiseLinearCaseSplit::setEquation( const Equation &equation )
{
    _equation = equation;
}

Equation PiecewiseLinearCaseSplit::getEquation() const
{
    return _equation;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
