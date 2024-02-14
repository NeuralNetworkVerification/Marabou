/*********************                                                        */
/*! \file PiecewiseLinearCaseSplit.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include <cstdio>

void PiecewiseLinearCaseSplit::storeBoundTightening( const Tightening &tightening )
{
    _bounds.append( tightening );
}

const List<Tightening> & PiecewiseLinearCaseSplit::getBoundTightenings() const
{
    return _bounds;
}

void PiecewiseLinearCaseSplit::addEquation( const Equation &equation )
{
	_equations.append( equation );
}

const List<Equation> & PiecewiseLinearCaseSplit::getEquations() const
{
	return _equations;
}

void PiecewiseLinearCaseSplit::dump( String &output ) const
{
    output = String( "\nDumping piecewise linear case split\n" );
    output += String( "\tBounds are:\n" );
    for ( const auto &bound : _bounds )
    {
        output += Stringf( "\t\tVariable: %u. New bound: %.2lf. Bound type: %s\n",
                           bound._variable, bound._value, bound._type == Tightening::LB ? "lower" : "upper" );
    }

    output += String( "\n\tEquations are:\n" );
    for ( const auto &equation : _equations )
    {
        output += String( "\t\t" );
        equation.dump();
    }
}

void PiecewiseLinearCaseSplit::dump() const
{
    String output;
    dump( output );
    printf( "%s", output.ascii() );
}

bool PiecewiseLinearCaseSplit::operator==( const PiecewiseLinearCaseSplit &other ) const
{
    return ( _bounds == other._bounds ) && ( _equations == other._equations );
}

void PiecewiseLinearCaseSplit::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    for ( auto &bound : _bounds )
    {
        if ( bound._variable == oldIndex )
            bound._variable = newIndex;
    }

    for ( auto &equation : _equations )
        equation.updateVariableIndex( oldIndex, newIndex );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
