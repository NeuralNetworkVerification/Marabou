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
#include "Map.h"
#include "Tightening.h"

Preprocessor::Preprocessor( InputQuery input )
{
	_input = input;
	_hasTightened = false;
}

InputQuery Preprocessor::getInputQuery()
{
	return _input;
}

void Preprocessor::tightenEquationsAndPL()
{
	while ( _hasTightened )
	{
		tightenBounds();
		tightenPL();
		_hasTightened = false; 
	}
}

void Preprocessor::tightenBounds() 
{
	double min = FloatUtils::negativeInfinity();
    double max = FloatUtils::infinity();

    for ( const auto &equation : _input.getEquations() )
    {
        bool sawUnboundedVariable = false;
        for ( const auto &addend : equation._addends )
        {
            // Look for unbounded variables
            if ( _input.getLowerBound( addend._variable ) == min ||
                 _input.getUpperBound( addend._variable ) == max )
            {
				_hasTightened = true;

                // If an equation has two addends that are unbounded on both sides,
                // no bounds can be tightened
                if ( _input.getLowerBound( addend._variable ) == min &&
                     _input.getUpperBound( addend._variable ) == max )
                {
                    // Give up on this equation
                    if ( sawUnboundedVariable )
					{
						_hasTightened = false;
                        break;
					}

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
                        scalarLB -= bounded._coefficient * _input.getLowerBound( bounded._variable );
                        scalarUB -= bounded._coefficient * _input.getUpperBound( bounded._variable );
                    }
                    if ( FloatUtils::isPositive( bounded._coefficient ) )
                    {
                        scalarLB -= bounded._coefficient * _input.getUpperBound( bounded._variable );
                        scalarUB -= bounded._coefficient * _input.getLowerBound( bounded._variable );
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

                if ( FloatUtils::gt( scalarLB, _input.getLowerBound( addend._variable ) ) )
                    _input.setLowerBound( addend._variable, scalarLB );
                if ( FloatUtils::lt( scalarUB, _input.getUpperBound( addend._variable ) ) )
                    _input.setUpperBound( addend._variable, scalarUB );
            }

            if ( FloatUtils::gt( _input.getLowerBound( addend._variable ),
                                 _input.getUpperBound( addend._variable ) ) )
                throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "Preprocessing bound error" );
        }
    }
}
	
void Preprocessor::tightenPL()
{
	for ( auto pl : _input.getPiecewiseLinearConstraints() )
	{
		for (auto var : pl->getParticipatingVariables() )
		{
			pl->preprocessBounds( var, _input.getLowerBound( var ), Tightening::LB );
			pl->preprocessBounds( var, _input.getUpperBound( var ), Tightening::UB );
		}
		pl->updateBounds();
		while ( !pl->getEntailedTightenings().empty() )
		{
			auto tighten = pl->getEntailedTightenings().peak();
			pl->tightenPL( tighten );
			if ( tighten._type == Tightening::LB )
				_input.setLowerBound( tighten._variable, tighten._value );
			else 
				_input.setUpperBound( tighten._variable, tighten._value );
			pl->getEntailedTightenings().pop();
		}
	}
}

void Preprocessor::eliminateVariables() 
{
	Map< unsigned, double > rmVariables;
	for ( unsigned i = 0; i < _input.getNumberOfVariables(); ++i )
	{
			if ( FloatUtils::areEqual( _input.getLowerBound( i ), _input.getUpperBound( i ) ) )
			{
				rmVariables[i] = _input.getLowerBound( i );
			}
	}
	int offset = 0;
	Map< unsigned, double > indexAssignment;
	for ( unsigned i = 0; i < _input.getNumberOfVariables(); ++i )
	{
		if ( rmVariables.exists( i ) )
		{
			++offset;
		}
		else 
			indexAssignment[i] = i - offset;
	}
	for ( auto &equation : _input.getEquations() )
	{
		int nRm = equation._addends.size(); 
		int changeLimit = equation._addends.size();
		//necessary to remove addends
		for ( auto addend = equation._addends.begin(); addend != equation._addends.end(); )
		{
			if ( changeLimit == 0 ) 
				break;

			if ( rmVariables.exists(  addend->_variable ) )
			{
				equation.setScalar( equation._scalar - addend->_coefficient * rmVariables.get( addend->_variable ) );
				auto temp = ++addend;
				--addend;
				equation._addends.erase( addend );
				addend = temp;
				++nRm;
			}
			else 
			{
				_input.setLowerBound( indexAssignment.get( addend->_variable ), _input.getLowerBound( addend->_variable ) );
				_input.setUpperBound( indexAssignment.get( addend->_variable ), _input.getUpperBound( addend->_variable ) );
				addend->_variable = indexAssignment.get( addend->_variable );
			}
			--changeLimit;
			++addend;
		}
		if ( nRm == 0 )
			_input.getEquations().erase( equation );
	}
	for ( auto pl : _input.getPiecewiseLinearConstraints() )
	{
		for ( auto var : pl->getParticipatingVariables() )
		{
			if ( indexAssignment.get( var ) == -1 )
				pl->eliminateVar( var, rmVariables.get( var ) );
			else if ( indexAssignment.get( var ) != var ) 
				pl->updateVarIndex( var, indexAssignment.get( var ) );
		}
	}
	_input.setNumberOfVariables( _input.getNumberOfVariables() - rmVariables.size() );
}


//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
