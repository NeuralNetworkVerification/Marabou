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
#include "Map.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "Tightening.h"

Preprocessor::Preprocessor( const InputQuery &query )
    : _preprocessed( query )
{
}

InputQuery Preprocessor::preprocess()
{
    /*
      Do the preprocessing steps:

      Until saturation:
        1. Tighten bounds using equations
        2. Tighten bounds using pl constraints

      Then, eliminate fixed variables.
    */

    bool continueTightening = true;
    while ( continueTightening )
    {
        continueTightening = tightenBounds();
        continueTightening = tightenPL() || continueTightening;
    }
	return _preprocessed;
}

InputQuery Preprocessor::getInputQuery()
{
	return _preprocessed;
}

bool Preprocessor::tightenBounds()
{
    bool tighterBoundFound = false;

	double min = FloatUtils::negativeInfinity();
    double max = FloatUtils::infinity();

    for ( const auto &equation : _preprocessed.getEquations() )
    {
        bool sawUnboundedVariable = false;
        for ( const auto &addend : equation._addends )
        {
            // Look for unbounded variables
            if ( _preprocessed.getLowerBound( addend._variable ) == min ||
                 _preprocessed.getUpperBound( addend._variable ) == max )
            {
                // If an equation has two addends that are unbounded on both sides,
                // no bounds can be tightened
                if ( _preprocessed.getLowerBound( addend._variable ) == min &&
                     _preprocessed.getUpperBound( addend._variable ) == max )
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
                bool validLB = true;
                bool validUB = true;

                for ( auto bounded : equation._addends )
                {
                    if ( addend._variable == bounded._variable )
                        continue;

                    if ( FloatUtils::isNegative( bounded._coefficient ) )
                    {
                        if ( validLB )
                        {
                            double boundedLB = _preprocessed.getLowerBound( bounded._variable );
                            if ( FloatUtils::isFinite( boundedLB ) )
                                scalarLB -= bounded._coefficient * boundedLB;
                            else
                                validLB = false;
                        }

                        if ( validUB )
                        {
                            double boundedUB = _preprocessed.getUpperBound( bounded._variable );
                            if ( FloatUtils::isFinite( boundedUB ) )
                                scalarUB -= bounded._coefficient * boundedUB;
                            else
                                validUB = false;
                        }
                    }

                    if ( FloatUtils::isPositive( bounded._coefficient ) )
                    {
                        if ( validLB )
                        {
                            double boundedUB = _preprocessed.getUpperBound( bounded._variable );
                            if ( FloatUtils::isFinite( boundedUB ) )
                                scalarLB -= bounded._coefficient * boundedUB;
                            else
                                validLB = false;
                        }

                        if ( validUB )
                        {
                            double boundedLB = _preprocessed.getLowerBound( bounded._variable );
                            if ( FloatUtils::isFinite( boundedLB ) )
                                scalarUB -= bounded._coefficient * boundedLB;
                            else
                                validUB = false;
                        }
                    }
                }

                // We know that lb < ax < ub. We want to divide by a, but we care about the sign
                // If a is positive: lb/a < x < ub/a
                // If a is negative: lb/a > x > ub/a
                if ( validLB )
                    scalarLB = scalarLB / addend._coefficient;
                if ( validUB )
                    scalarUB = scalarUB / addend._coefficient;

                if ( FloatUtils::isNegative( addend._coefficient ) )
                {
                    double temp = scalarUB;
                    scalarUB = scalarLB;
                    scalarLB = temp;

                    bool tempValid = validUB;
                    validUB = validLB;
                    validLB = tempValid;
                }

                if ( validLB && FloatUtils::gt( scalarLB, _preprocessed.getLowerBound( addend._variable ) ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setLowerBound( addend._variable, scalarLB );
                }

                if ( validUB && FloatUtils::lt( scalarUB, _preprocessed.getUpperBound( addend._variable ) ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setUpperBound( addend._variable, scalarUB );
                }
            }

            if ( FloatUtils::gt( _preprocessed.getLowerBound( addend._variable ),
                                 _preprocessed.getUpperBound( addend._variable ) ) )
                throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "Preprocessing bound error" );
        }
    } 

    return tighterBoundFound;
}

bool Preprocessor::tightenPL()
{
    bool tighterBoundFound = false;

	for ( auto &constraint : _preprocessed.getPiecewiseLinearConstraints() )
	{
		for ( unsigned variable : constraint->getParticipatingVariables() )
		{
			constraint->notifyLowerBound( variable, _preprocessed.getLowerBound( variable ) );
			constraint->notifyUpperBound( variable, _preprocessed.getUpperBound( variable ) );
		}

        List<Tightening> tightenings;
        constraint->getEntailedTightenings( tightenings );

        for ( const auto &tightening : tightenings )
		{
			constraint->tightenPL( tightening, tightenings );
			if ( ( tightening._type == Tightening::LB ) &&
                 FloatUtils::gt( tightening._value, _preprocessed.getLowerBound( tightening._variable ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setLowerBound( tightening._variable, tightening._value );
            }

			else if ( ( tightening._type == Tightening::UB ) &&
				 FloatUtils::lt( tightening._value, _preprocessed.getUpperBound( tightening._variable ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setUpperBound( tightening._variable, tightening._value );
            }
		}
	}

    return tighterBoundFound;
}

void Preprocessor::eliminateVariables()
{
	Map< unsigned, double > rmVariables;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
            rmVariables[i] = _preprocessed.getLowerBound( i );
	}

	int offset = 0;
	Map<unsigned, double> indexAssignment;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
		if ( rmVariables.exists( i ) )
		{
			++offset;
			indexAssignment[i] = -1;
		}
		else
			indexAssignment[i] = i - offset;
	}

	for ( auto &equation : _preprocessed.getEquations() )
	{
		int nRm = equation._addends.size();
		int changeLimit = equation._addends.size();
		//necessary to remove addends
		for ( auto addend = equation._addends.begin(); addend != equation._addends.end(); )
		{
			if ( changeLimit == 0 )
				break;

			if ( rmVariables.exists( addend->_variable ) )
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
				addend->_variable = indexAssignment.get( addend->_variable );
				++addend;
			}

			--changeLimit;
		}
		if ( equation._addends.size() == 0 )
		{
			if ( !FloatUtils::isZero( equation._scalar ) )
                throw ReluplexError( ReluplexError::EQUATION_INVALID, "variables eliminated incorrectly" );
			_preprocessed.getEquations().erase( equation );
		}
	}
	for ( auto key : indexAssignment.keys() )
	{
		if ( indexAssignment.get( key ) != -1 )
		{
			_preprocessed.setLowerBound( indexAssignment.get( key ),
                                         _preprocessed.getLowerBound( key ) );
			_preprocessed.setUpperBound( indexAssignment.get( key ),
                                         _preprocessed.getUpperBound( key ) );
		}
	}
	auto const  PLConstraints = _preprocessed.getPiecewiseLinearConstraints();
	for ( auto pl : PLConstraints )
	{
		auto participatingVar = pl->getParticipatingVariables();
		for ( auto key : indexAssignment.keys() )
		{
			if ( participatingVar.exists( key ) )
			{
				if ( indexAssignment[key] == -1 )
					pl->eliminateVariable( key, rmVariables.get( key ) );
				else if ( indexAssignment.get( key ) != key )
					pl->updateVariableIndex( key, indexAssignment.get( key ) );
			}
		}
		if ( pl->_removePL )
		{
			//_preprocessed.getPiecewiseLinearConstraints().erase( pl );
			//TODO: remove PL
		}
	}
	_preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - rmVariables.size() );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
