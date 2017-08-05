/*********************                                                        */
/*! \file Preprocessor.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "FloatUtils.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

void Preprocessor::tightenBounds( InputQuery &input )
{
	double min = -DBL_MAX;
    double max = DBL_MAX;

    for ( const auto &equation : input.getEquations() )
    {
        bool sawUnboundedVariable = false;
        for ( const auto &addend : equation._addends )
        {
            // Look for unbounded variables
            if ( input.getLowerBound( addend._variable ) == min ||
                 input.getUpperBound( addend._variable ) == max )
            {

                // If an equation has two addends that are unbounded on both sides,
                // no bounds can be tightened
                if ( input.getLowerBound( addend._variable ) == min &&
                     input.getUpperBound( addend._variable ) == max )
                {
                    // Give up on this equation
                    if ( sawUnboundedVariable )
                        break;

                    sawUnboundedVariable = true;
                }

                // The equation is of the form ax + sum (bi * xi) = c,
                // or: ax = c - sum (bi * xi)

                // We first compute the lower and upper bounds for the expression c - sum (bi * xi)
                double scalarUB = equation._scalar;
                double scalarLB = equation._scalar;
                for ( auto bounded : equation._addends )
                {
                    if ( addend._variable == bounded._variable )
                        continue;

                    if ( FloatUtils::isNegative( bounded._coefficient ) )
                    {
                        scalarLB -= bounded._coefficient * input.getLowerBound( bounded._variable );
                        scalarUB -= bounded._coefficient * input.getUpperBound( bounded._variable );
                    }
                    if ( FloatUtils::isPositive( bounded._coefficient ) )
                    {
                        scalarLB -= bounded._coefficient * input.getUpperBound( bounded._variable );
                        scalarUB -= bounded._coefficient * input.getLowerBound( bounded._variable );
                    }
                }

                // We know that lb < ax < ub. We want to divide by a, but we care about the sign
                // If a is positive: lb/a < x < ub/a
                // If a is negative: lb/a > x > ub/a
                scalarLB = scalarLB / addend._coefficient;
                scalarUB = scalarUB / addend._coefficient;

                if ( FloatUtils::isNegative( addend._coefficient ) )
                {
                    double temp = scalarUB;
                    scalarUB = scalarLB;
                    scalarLB = temp;
                }

                if ( FloatUtils::gt( scalarLB, input.getLowerBound( addend._variable ) ) )
                    input.setLowerBound( addend._variable, scalarLB );
                if ( FloatUtils::lt( scalarUB, input.getUpperBound( addend._variable ) ) )
                    input.setUpperBound( addend._variable, scalarUB );
            }

            if ( FloatUtils::gt( input.getLowerBound( addend._variable ),
                                 input.getUpperBound( addend._variable ) ) )
                throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "Preprocessing bound error" );
        }
    }
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
