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

InputQuery Preprocessor::preprocess( const InputQuery &query )
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

void Preprocessor::eliminateVariables()
{
// 	Map< unsigned, double > rmVariables;
// 	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
// 	{
//         if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
//             rmVariables[i] = _preprocessed.getLowerBound( i );
// 	}

// 	int offset = 0;
// 	Map<unsigned, double> indexAssignment;
// 	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
// 	{
// 		if ( rmVariables.exists( i ) )
// 		{
// 			++offset;
// 			indexAssignment[i] = -1;
// 		}
// 		else
// 			indexAssignment[i] = i - offset;
// 	}

// 	for ( auto &equation : _preprocessed.getEquations() )
// 	{
// 		int nRm = equation._addends.size();
// 		int changeLimit = equation._addends.size();
// 		//necessary to remove addends
// 		for ( auto addend = equation._addends.begin(); addend != equation._addends.end(); )
// 		{
// 			if ( changeLimit == 0 )
// 				break;

// 			if ( rmVariables.exists( addend->_variable ) )
// 			{
// 				equation.setScalar( equation._scalar - addend->_coefficient * rmVariables.get( addend->_variable ) );
// 				auto temp = ++addend;
// 				--addend;
// 				equation._addends.erase( addend );
// 				addend = temp;
// 				++nRm;
// 			}
// 			else
// 			{
// 				addend->_variable = indexAssignment.get( addend->_variable );
// 				++addend;
// 			}

// 			--changeLimit;
// 		}
// 		if ( equation._addends.size() == 0 )
// 		{
// 			if ( !FloatUtils::isZero( equation._scalar ) )
//                 throw ReluplexError( ReluplexError::EQUATION_INVALID, "variables eliminated incorrectly" );
// 			_preprocessed.getEquations().erase( equation );
// 		}
// 	}
// 	for ( auto key : indexAssignment.keys() )
// 	{
// 		if ( indexAssignment.get( key ) != -1 )
// 		{
// 			_preprocessed.setLowerBound( indexAssignment.get( key ),
//                                          _preprocessed.getLowerBound( key ) );
// 			_preprocessed.setUpperBound( indexAssignment.get( key ),
//                                          _preprocessed.getUpperBound( key ) );
// 		}
// 	}
// 	auto const  PLConstraints = _preprocessed.getPiecewiseLinearConstraints();
// 	for ( auto pl : PLConstraints )
// 	{
// 		auto participatingVar = pl->getParticipatingVariables();
// 		for ( auto key : indexAssignment.keys() )
// 		{
// 			if ( participatingVar.exists( key ) )
// 			{
// 				if ( indexAssignment[key] == -1 )
// 					pl->eliminateVariable( key, rmVariables.get( key ) );
// 				else if ( indexAssignment.get( key ) != key )
// 					pl->updateVariableIndex( key, indexAssignment.get( key ) );
// 			}
// 		}
// 		if ( pl->_removePL )
// 		{
// 			//_preprocessed.getPiecewiseLinearConstraints().erase( pl );
// 			//TODO: remove PL
// 		}
// 	}
// 	_preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - rmVariables.size() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
