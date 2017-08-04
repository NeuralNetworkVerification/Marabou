/*********************                                                        */
/*! \file InputQuery.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "InputQuery.h"
#include "MStringf.h"
#include "ReluplexError.h"

#include <cfloat>

InputQuery::InputQuery()
{
}

InputQuery::~InputQuery()
{
    auto it = _plConstraints.begin();
    while ( it != _plConstraints.end() )
    {
        delete *it;
        ++it;
    }
}

void InputQuery::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

void InputQuery::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _upperBounds[variable] = bound;
}

void InputQuery::addEquation( const Equation &equation )
{
    _equations.append( equation );
}

double InputQuery::getNumberOfVariables() const
{
    return _numberOfVariables;
}

double InputQuery::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return -DBL_MAX;

    return _lowerBounds.get( variable );
}

double InputQuery::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return DBL_MAX;

    return _upperBounds.get( variable );
}

const List<Equation> &InputQuery::getEquations() const
{
    return _equations;
}

void InputQuery::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double InputQuery::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw ReluplexError( ReluplexError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                             Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void InputQuery::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.append( constraint );
}

const List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints() const
{
    return _plConstraints;
}

void InputQuery::preprocessBounds() 
{
	double min = -DBL_MAX;
	double max = DBL_MAX;

	for ( auto equation : _equations )
	{
		for ( auto addend : equation._addends )
		{
				if ( getLowerBound( addend._variable ) == min || getUpperBound( addend._variable ) == max )
				{

					double scalarUB = equation._scalar;
					double scalarLB = equation._scalar;

					for ( auto bounded : equation._addends )
					{
						if ( addend._variable == bounded._variable ) continue;
	
						if ( bounded._coefficient < 0 )
						{
							scalarLB -= bounded._coefficient * getLowerBound( bounded._variable );
							scalarUB -= bounded._coefficient * getUpperBound( bounded._variable );
						}
						else if ( bounded._coefficient > 0 )
						{
							scalarLB -= bounded._coefficient * getUpperBound( bounded._variable );
							scalarUB -= bounded._coefficient * getLowerBound( bounded._variable );
						}
					}


					if ( scalarLB > getLowerBound( addend._variable ) )
							setLowerBound( addend._variable, scalarLB );
					if ( scalarUB < getUpperBound( addend._variable ) )
							setUpperBound( addend._variable, scalarUB );
			}


			if ( getLowerBound( addend._variable) > getUpperBound( addend._variable) )
				throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "preprocessing bound error" );
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
