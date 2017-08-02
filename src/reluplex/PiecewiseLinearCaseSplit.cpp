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

// bool PiecewiseLinearCaseSplit::valid( const Map<unsigned, double> &lowerBounds, const Map<unsigned, double> &upperBounds ) const
// {
//     for ( const auto &tightening : _bounds )
//     {
//         switch ( tightening._type )
//         {
//             unsigned variable = tightening._variable;
//             double value = tightening._value;
//             case Tightening::BoundType::LB:
//                 if ( upperBounds.exists( var ) && FloatUtils::lt( upperBounds[var], value ) )
//                     return false;
//                 break;
//             case Tightening::BoundType::UB:
//                 if ( lowerBounds.exists( var ) && FloatUtils::lt( value, lowerBounds[var] ) )
//                     return false;
//                 break;
// 	    }
//     }
//     return true;
// }

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
