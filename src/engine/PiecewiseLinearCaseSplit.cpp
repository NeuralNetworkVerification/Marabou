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
#include <cstdio>

void PiecewiseLinearCaseSplit::storeBoundTightening( const Tightening &tightening )
{
    _bounds.append( tightening );
}

List<Tightening> PiecewiseLinearCaseSplit::getBoundTightenings() const
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

void PiecewiseLinearCaseSplit::setConstraintAndSplitID( unsigned constraintID, unsigned splitID )
{
    _constraintID = constraintID;
    _splitID = splitID;
}

void PiecewiseLinearCaseSplit::setFactsConstraintAndSplitID()
{
    // Guy: why not move this functionality to when bounds and equations are stored in the case split?
    for ( auto &bound: _bounds )
        bound.setCausingConstraintAndSplitID( _constraintID, _splitID );

    for ( auto &equation: _equations )
        equation.setCausingConstraintAndSplitID( _constraintID, _splitID );
}

unsigned PiecewiseLinearCaseSplit::getConstraintID() const
{
    return _constraintID;
}

unsigned PiecewiseLinearCaseSplit::getSplitID() const
{
    return _splitID;
}

void PiecewiseLinearCaseSplit::addExplanation( unsigned causeID )
{
    for( auto &bound: _bounds )
        bound.addExplanation( causeID );

    for ( auto &equation: _equations )
        equation.addExplanation( causeID );
}

void PiecewiseLinearCaseSplit::dump() const
{
    printf( "\nDumping piecewise linear case split\n" );
    printf( "\tBounds are:\n" );
    for ( const auto &bound : _bounds )
    {
        printf( "\t\tVariable: %u. New bound: %.2lf. Bound type: %s\n",
                bound._variable, bound._value, bound._type == Tightening::LB ? "lower" : "upper" );
    }

    printf( "\n\tEquations are:\n" );
    for ( const auto &equation : _equations )
    {
        printf( "\t\t" );
        equation.dump();
    }
}

bool PiecewiseLinearCaseSplit::operator==( const PiecewiseLinearCaseSplit &other ) const
{
    if ( _splitID == other._splitID && _constraintID == other._constraintID )
      return true;
    return ( _bounds == other._bounds ) && ( _equations == other._equations );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
