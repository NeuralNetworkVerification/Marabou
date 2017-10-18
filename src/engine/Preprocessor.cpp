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
#include "Statistics.h"
#include "Tightening.h"

Preprocessor::Preprocessor()
    : _statistics( NULL )
{
}

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

        if ( _statistics )
            _statistics->ppIncNumTighteningIterations();
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

            if ( validLB && FloatUtils::gt( scalarLB, _preprocessed.getLowerBound( varBeingTightened._variable ),
                                            GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
            {
                tighterBoundFound = true;
                _preprocessed.setLowerBound( varBeingTightened._variable, scalarLB );
            }

            if ( validUB && FloatUtils::lt( scalarUB, _preprocessed.getUpperBound( varBeingTightened._variable ),
                                            GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
            {
                tighterBoundFound = true;
                _preprocessed.setUpperBound( varBeingTightened._variable, scalarUB );
            }

            if ( FloatUtils::gt( _preprocessed.getLowerBound( varBeingTightened._variable ),
                                 _preprocessed.getUpperBound( varBeingTightened._variable ),
                                 GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
            {
                throw InfeasibleQueryException();
            }
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
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
            _fixedVariables[i] = _preprocessed.getLowerBound( i );
	}

    // If there's nothing to eliminate, we're done
    if ( _fixedVariables.empty() )
        return;

    if ( _statistics )
        _statistics->ppSetNumEliminatedVars( _fixedVariables.size() );

    // Compute the new variable indices, after the elimination of fixed variables
 	int offset = 0;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( _fixedVariables.exists( i ) )
            ++offset;
        else
            _oldIndexToNewIndex[i] = i - offset;
	}

    // Next, eliminate the fixed variables from the equations
    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    Set<unsigned> auxiliaryVariables;
    while ( equation != equations.end() )
	{
        // Each equation is of the form sum(addends) = scalar. So, any fixed variable
        // needs to be subtracted from the scalar.
        List<Equation::Addend>::iterator addend = equation->_addends.begin();
        while ( addend != equation->_addends.end() )
        {
            if ( _fixedVariables.exists( addend->_variable ) )
            {
                // Addend has to go...
                double constant = _fixedVariables.at( addend->_variable ) * addend->_coefficient;
                equation->_scalar -= constant;
                addend = equation->_addends.erase( addend );
            }
            else
            {
                // Adjust the addend's variable index
                addend->_variable = _oldIndexToNewIndex.at( addend->_variable );
                ++addend;
            }
        }

        // If the auxiliary variable has been eliminated, pick a new one,
        // unless the equation has no addends left
        if ( !equation->_addends.empty() )
        {
            if ( _fixedVariables.exists( equation->_auxVariable ) )
            {
                bool found = false;
                addend = equation->_addends.begin();
                while ( !found && ( addend != equation->_addends.end() ) )
                {
                    // A variable can't be aux for two equations
                    if ( !auxiliaryVariables.exists( addend->_variable ) )
                    {
                        // This variable is free, grab it
                        equation->_auxVariable = addend->_variable;
                        found = true;
                    }

                    ++addend;
                }

                if ( !found )
                {
                    // Couldn't find a new auxiliary variable!
                    throw ReluplexError( ReluplexError::PREPROCESSOR_CANT_FIND_NEW_AUXILIARY_VAR );
                }
            }
            else
            {
                equation->_auxVariable = _oldIndexToNewIndex.at( equation->_auxVariable );
            }

            auxiliaryVariables.insert( equation->_auxVariable );
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

    // Let the piecewise-linear constraints know of any eliminated variables, and remove
    // the constraints themselves if they become obsolete.
    List<PiecewiseLinearConstraint *> &constraints( _preprocessed.getPiecewiseLinearConstraints() );
    List<PiecewiseLinearConstraint *>::iterator constraint = constraints.begin();
    while ( constraint != constraints.end() )
    {
        List<unsigned> participatingVariables = (*constraint)->getParticipatingVariables();
        for ( unsigned variable : participatingVariables )
        {
            if ( _fixedVariables.exists( variable ) )
                (*constraint)->eliminateVariable( variable, _fixedVariables.at( variable ) );
        }

        if ( (*constraint)->constraintObsolete() )
        {
            _statistics->ppIncNumConstraintsRemoved();
            constraint = constraints.erase( constraint );
        }
        else
            ++constraint;
	}

    // Let the remaining piecewise-lienar constraints know of any changes in indices.
    for ( const auto &constraint : constraints )
	{
		List<unsigned> participatingVariables = constraint->getParticipatingVariables();
        for ( unsigned variable : participatingVariables )
        {
            if ( _oldIndexToNewIndex.at( variable ) != variable )
                constraint->updateVariableIndex( variable, _oldIndexToNewIndex.at( variable ) );
        }
	}

    // Update the lower/upper bound maps
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( _fixedVariables.exists( i ) )
            continue;

        _preprocessed.setLowerBound( _oldIndexToNewIndex.at( i ), _preprocessed.getLowerBound( i ) );
        _preprocessed.setUpperBound( _oldIndexToNewIndex.at( i ), _preprocessed.getUpperBound( i ) );
	}

    // Adjust the number of variables in the query
    _preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - _fixedVariables.size() );
}

bool Preprocessor::variableIsFixed( unsigned index ) const
{
    return _fixedVariables.exists( index );
}

double Preprocessor::getFixedValue( unsigned index ) const
{
    return _fixedVariables.at( index );
}

unsigned Preprocessor::getNewIndex( unsigned oldIndex ) const
{
    if ( _oldIndexToNewIndex.exists( oldIndex ) )
        return _oldIndexToNewIndex.at( oldIndex );

    return oldIndex;
}

void Preprocessor::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
