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

void PiecewiseLinearCaseSplit::storeBoundTightening( const Bound &bound )
{
    _bounds.append( bound );
}

List<PiecewiseLinearCaseSplit::Bound> PiecewiseLinearCaseSplit::getBoundTightenings() const
{
    return _bounds;
}

void PiecewiseLinearCaseSplit::addEquation( const Equation &equation )
{
	_equations.append( equation );
}


List<Equation> PiecewiseLinearCaseSplit::getEquations() const
{
	return _equations;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
