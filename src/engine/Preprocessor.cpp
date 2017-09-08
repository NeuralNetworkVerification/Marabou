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
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "Map.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "Tightening.h"

InputQuery Preprocessor::preprocess( const InputQuery &query, bool attemptVariableElimination )
{
    _preprocessed = query;

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
        continueTightening = processEquations();
        continueTightening = processConstraints() || continueTightening;
    }

    if ( attemptVariableElimination )
        eliminateFixedVariables();

	return _preprocessed;
}

bool Preprocessor::processEquations()
{
    bool tighterBoundFound = false;

    for ( const auto &equation : _preprocessed.getEquations() )
    {
        for ( const auto &varBeingTightened : equation._addends )
        {
            // The equation is of the form a * varBeingTightened + sum (bi * xi) = c,
            // or: a * varBeingTightened = c - sum (bi * xi)

            // We first compute the lower and upper bounds for the expression c - sum (bi * xi)
            double scalarUB = equation._scalar;
            double scalarLB = equation._scalar;
            bool validLB = true;
            bool validUB = true;

            for ( auto addend : equation._addends )
            {
                if ( varBeingTightened._variable == addend._variable )
                    continue;

                if ( FloatUtils::isNegative( addend._coefficient ) )
                {
                    if ( validLB )
                    {
                        double addendLB = _preprocessed.getLowerBound( addend._variable );
                        if ( FloatUtils::isFinite( addendLB ) )
                            scalarLB -= addend._coefficient * addendLB;
                        else
                            validLB = false;
                    }

                    if ( validUB )
                    {
                        double addendUB = _preprocessed.getUpperBound( addend._variable );
                        if ( FloatUtils::isFinite( addendUB ) )
                            scalarUB -= addend._coefficient * addendUB;
                        else
                            validUB = false;
                    }
                }

                if ( FloatUtils::isPositive( addend._coefficient ) )
                {
                    if ( validLB )
                    {
                        double addendUB = _preprocessed.getUpperBound( addend._variable );
                        if ( FloatUtils::isFinite( addendUB ) )
                            scalarLB -= addend._coefficient * addendUB;
                        else
                            validLB = false;
                    }

                    if ( validUB )
                    {
                        double addendLB = _preprocessed.getLowerBound( addend._variable );
                        if ( FloatUtils::isFinite( addendLB ) )
                            scalarUB -= addend._coefficient * addendLB;
                        else
                            validUB = false;
                    }
                }
            }

            // We know that lb < a * varBeingTightened < ub. We want to divide by a, but we care about the sign
            // If a is positive: lb/a < x < ub/a
            // If a is negative: lb/a > x > ub/a
            if ( validLB )
                scalarLB = scalarLB / varBeingTightened._coefficient;
            if ( validUB )
                scalarUB = scalarUB / varBeingTightened._coefficient;

            if ( FloatUtils::isNegative( varBeingTightened._coefficient ) )
            {
                double temp = scalarUB;
                scalarUB = scalarLB;
                scalarLB = temp;

                bool tempValid = validUB;
                validUB = validLB;
                validLB = tempValid;
            }

            if ( validLB && FloatUtils::gt( scalarLB, _preprocessed.getLowerBound( varBeingTightened._variable ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setLowerBound( varBeingTightened._variable, scalarLB );
            }

            if ( validUB && FloatUtils::lt( scalarUB, _preprocessed.getUpperBound( varBeingTightened._variable ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setUpperBound( varBeingTightened._variable, scalarUB );
            }

            if ( FloatUtils::gt( _preprocessed.getLowerBound( varBeingTightened._variable ),
                                 _preprocessed.getUpperBound( varBeingTightened._variable ) ) )
                throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "Preprocessing bound error" );
        }
    }

    return tighterBoundFound;
}

bool Preprocessor::processConstraints()
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

void Preprocessor::eliminateFixedVariables()
{
    // First, collect the variables that have become fixed, and their fixed values
	Map<unsigned, double> fixedVariables;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
            fixedVariables[i] = _preprocessed.getLowerBound( i );
	}

    // If there's nothing to eliminate, we're done
    if ( fixedVariables.empty() )
        return;

    // Compute the new variable indices, after the elimination of fixed variables
    Map<unsigned, unsigned> oldIndexToNewIndex;

 	int offset = 0;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
		if ( fixedVariables.exists( i ) )
			++offset;
		else
			fixedVariables[i] = i - offset;
	}

    // Next, eliminate the fixed variables from the equations
    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    while ( equation != equations.end() )
	{
        // Each equation is of the form sum(addends) = scalar. So, any fixed variable
        // needs to be subtracted from the scalar.
        List<Equation::Addend>::iterator addend = equation->_addends.begin();
        while ( addend != equation->_addends.end() )
        {
            if ( fixedVariables.exists( addend->_variable ) )
            {
                // Addend has to go...
                double constant = fixedVariables[addend->_variable] * addend->_coefficient;
                equation->_scalar -= constant;
                addend = equation->_addends.erase( addend );
            }
            else
            {
                // Adjust the addend's variable index
                addend->_variable = oldIndexToNewIndex[addend->_variable];
                ++addend;
            }
        }

        // If all the addends have been removed, we remove the entire equation.
        // Overwise, we are done here.
        if ( equation->_addends.empty() )
        {
            // No addends left, scalar should be 0
            if ( !FloatUtils::isZero( equation->_scalar ) )
                throw InfeasibleQueryException();
            else
                equation = equations.erase( equation );
        }
        else
            ++equation;
	}

    // Let the piecewise-lienar constraints know of any eliminated variables and
    // change in indices.
    for ( const auto &constraint : _preprocessed.getPiecewiseLinearConstraints() )
	{
		List<unsigned> participatingVariables = constraint->getParticipatingVariables();
        for ( unsigned variable : participatingVariables )
        {
            if ( fixedVariables.exists( variable ) )
                constraint->eliminateVariable( variable, fixedVariables[variable] );
            else if ( oldIndexToNewIndex[variable] != variable )
                constraint->updateVariableIndex( variable, oldIndexToNewIndex[variable] );
        }
	}

    // Update the lower/upper bound maps
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( fixedVariables.exists( i ) )
            continue;

        _preprocessed.setLowerBound( oldIndexToNewIndex[i], _preprocessed.getLowerBound( i ) );
        _preprocessed.setUpperBound( oldIndexToNewIndex[i], _preprocessed.getUpperBound( i ) );
	}

    // Adjust the number of variables in the query
    _preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - fixedVariables.size() );

    // TODO: need to maintain the mappings, for extracting the solutions
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
