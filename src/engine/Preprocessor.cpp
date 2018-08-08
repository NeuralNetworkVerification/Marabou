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

#include "Debug.h"
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
      Initial work: if needed, have the PL constraints add their additional
      equations to the pool.
    */
    if ( GlobalConfiguration::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS )
        addPlAuxiliaryEquations();

    /*
      Next, make sure all equations are of type EQUALITY. If not, turn them
      into one.
    */
    makeAllEquationsEqualities();

    /*
      Do the preprocessing steps:

      Until saturation:
        1. Tighten bounds using equations
        2. Tighten bounds using pl constraints
        3. Unify identical variables

      Then, eliminate fixed variables.
    */

    bool continueTightening = true;
    while ( continueTightening )
    {
        numericalStability();
        continueTightening = processEquations();
        numericalStability();
        continueTightening = processConstraints() || continueTightening;
        numericalStability();
        continueTightening = processIdenticalVariables() || continueTightening;
        numericalStability();

        if ( _statistics )
            _statistics->ppIncNumTighteningIterations();
    }

    if ( attemptVariableElimination )
        eliminateVariables();

    DEBUG({
            // For now, assume merged and fixed variable sets are disjoint
            for ( const auto &fixed : _fixedVariables )
                ASSERT( !_mergedVariables.exists( fixed.first ) );
          });

    return _preprocessed;
}

void Preprocessor::makeAllEquationsEqualities()
{
    for ( auto &equation : _preprocessed.getEquations() )
    {
        if ( equation._type == Equation::EQ )
            continue;

        unsigned auxVariable = _preprocessed.getNumberOfVariables();
        _preprocessed.setNumberOfVariables( auxVariable + 1 );

        // Auxiliary variables are always added with coefficient 1
        if ( equation._type == Equation::GE )
            _preprocessed.setUpperBound( auxVariable, 0 );
        else
            _preprocessed.setLowerBound( auxVariable, 0 );

        equation._type = Equation::EQ;

        equation.addAddend( 1, auxVariable );
    }
}

bool Preprocessor::processEquations()
{
    enum {
        ZERO = 0,
        POSITIVE = 1,
        NEGATIVE = 2,
        INFINITE = 3,
    };

    bool tighterBoundFound = false;

    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    while ( equation != equations.end() )
    {
        // The equation is of the form sum (ci * xi) - b ? 0
        Equation::EquationType type = equation->_type;

        unsigned maxVar = equation->_addends.begin()->_variable;
        for ( const auto &addend : equation->_addends )
        {
            if ( addend._variable > maxVar )
                maxVar = addend._variable;
        }

        ++maxVar;

        double *ciTimesLb = new double[maxVar];
        double *ciTimesUb = new double[maxVar];
        char *ciSign = new char[maxVar];

        Set<unsigned> excludedFromLB;
        Set<unsigned> excludedFromUB;

        unsigned xi;
        double xiLB;
        double xiUB;
        double ci;
        double lowerBound;
        double upperBound;
        bool validLb;
        bool validUb;

        // The first goal is to compute the LB and UB of: sum (ci * xi) - b
        // For this we first identify unbounded variables
        double auxLb = -equation->_scalar;
        double auxUb = -equation->_scalar;
        for ( const auto &addend : equation->_addends )
        {
            ci = addend._coefficient;
            xi = addend._variable;

            if ( FloatUtils::isZero( ci ) )
            {
                ciSign[xi] = ZERO;
                ciTimesLb[xi] = 0;
                ciTimesUb[xi] = 0;
                continue;
            }

            ciSign[xi] = FloatUtils::isPositive( ci ) ? POSITIVE : NEGATIVE;

            xiLB = _preprocessed.getLowerBound( xi );
            xiUB = _preprocessed.getUpperBound( xi );

            if ( FloatUtils::isFinite( xiLB ) )
            {
                ciTimesLb[xi] = ci * xiLB;
                if ( ciSign[xi] == POSITIVE )
                    auxLb += ciTimesLb[xi];
                else
                    auxUb += ciTimesLb[xi];
            }
            else
            {
                if ( ci > 0 )
                    excludedFromLB.insert( xi );
                else
                    excludedFromUB.insert( xi );
            }

            if ( FloatUtils::isFinite( xiUB ) )
            {
                ciTimesUb[xi] = ci * xiUB;
                if ( ciSign[xi] == POSITIVE )
                    auxUb += ciTimesUb[xi];
                else
                    auxLb += ciTimesUb[xi];
            }
            else
            {
                if ( ci > 0 )
                    excludedFromUB.insert( xi );
                else
                    excludedFromLB.insert( xi );
            }
        }

        // Now, go over each addend in sum (ci * xi) - b ? 0, and see what can be done
        for ( const auto &addend : equation->_addends )
        {
            // bool learnedLower = false;
            // bool learnedUpper = false;

            ci = addend._coefficient;
            xi = addend._variable;

            // If ci = 0, nothing to do.
            if ( ciSign[xi] == ZERO )
                continue;

            /*
              The expression for xi is:

                   xi ? ( -1/ci ) * ( sum_{j\neqi} ( cj * xj ) - b )

              We use the previously computed auxLb and auxUb and adjust them because
              xi is removed from the sum. We also need to pay attention to the sign of ci,
              and to the presence of infinite bounds.

              Assuming "?" stands for equality, we can compute a LB if:
                1. ci is negative, and no vars except xi were excluded from the auxLb
                2. ci is positive, and no vars except xi were excluded from the auxUb

              And vice-versa for UB.

              In case "?" is GE or LE, only one direction can be computed.
            */
            if ( ciSign[xi] == NEGATIVE )
            {
                validLb =
                    ( ( type == Equation::LE ) || ( type == Equation::EQ ) )
                    &&
                    ( excludedFromLB.empty() ||
                      ( excludedFromLB.size() == 1 && excludedFromLB.exists( xi ) ) );
                validUb =
                    ( ( type == Equation::GE ) || ( type == Equation::EQ ) )
                    &&
                    ( excludedFromUB.empty() ||
                      ( excludedFromUB.size() == 1 && excludedFromUB.exists( xi ) ) );
            }
            else
            {
                validLb =
                    ( ( type == Equation::GE ) || ( type == Equation::EQ ) )
                    &&
                    ( excludedFromUB.empty() ||
                      ( excludedFromUB.size() == 1 && excludedFromUB.exists( xi ) ) );
                validUb =
                    ( ( type == Equation::LE ) || ( type == Equation::EQ ) )
                    &&
                    ( excludedFromLB.empty() ||
                      ( excludedFromLB.size() == 1 && excludedFromLB.exists( xi ) ) );
            }

            // Now compute the actual bounds and see if they are tighter
            if ( validLb )
            {
                if ( ciSign[xi] == NEGATIVE )
                {
                    lowerBound = auxLb;
                    if ( !excludedFromLB.exists( xi ) )
                        lowerBound -= ciTimesUb[xi];
                }
                else
                {
                    lowerBound = auxUb;
                    if ( !excludedFromUB.exists( xi ) )
                        lowerBound -= ciTimesUb[xi];
                }

                lowerBound /= -ci;

                if ( lowerBound > _preprocessed.getLowerBound( xi ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setLowerBound( xi, lowerBound );

                    // learnedLower = true;
                }
            }

            if ( validUb )
            {
                if ( ciSign[xi] == NEGATIVE )
                {
                    upperBound = auxUb;
                    if ( !excludedFromUB.exists( xi ) )
                        upperBound -= ciTimesLb[xi];
                }
                else
                {
                    upperBound = auxLb;
                    if ( !excludedFromLB.exists( xi ) )
                        upperBound -= ciTimesLb[xi];
                }

                upperBound /= -ci;

                if ( upperBound < _preprocessed.getUpperBound( xi ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setUpperBound( xi, upperBound );

                    // learnedUpper = true;
                }
            }


            // if ( learnedUpper || learnedLower )
            // {
            //     if ( learnedUpper )
            //         printf( "Learned lower: %.15lf\n", lowerBound );
            //     if ( learnedLower )
            //         printf( "Learned upper: %.15lf\n", upperBound );

            //     if ( FloatUtils::areDisequal( lowerBound, upperBound ) )
            //     {
            //         printf( "Warning: DISEQUAL!\n" );
            //     }
            // }

            if ( FloatUtils::gt( _preprocessed.getLowerBound( xi ), _preprocessed.getUpperBound( xi ), 1e-5 ) )
            {
                printf( "Throwing InfeasibleQueryException. Variable = x%u, Lb = %.15lf, Ub = %.15lf\n",
                        xi,
                        _preprocessed.getLowerBound( xi ),
                        _preprocessed.getUpperBound( xi ) );

                delete[] ciTimesLb;
                delete[] ciTimesUb;
                delete[] ciSign;

                throw InfeasibleQueryException();
            }
        }

        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( xi ),
                                   _preprocessed.getUpperBound( xi ) ) )
        {
            _preprocessed.setUpperBound( xi, _preprocessed.getLowerBound( xi ) );
        }

        delete[] ciTimesLb;
        delete[] ciTimesUb;
        delete[] ciSign;

        /*
          When finished processing the equation, check if all variables are fixed.
          If so, if the equation holds, it can be removed.
          Otherwise, the query is infeasible.
        */

        bool allFixed = true;
        for ( const auto &addend : equation->_addends )
        {
            unsigned var = addend._variable;
            if ( !FloatUtils::areEqual( _preprocessed.getLowerBound( var ),
                                        _preprocessed.getUpperBound( var ) ) )
            {
                allFixed = false;
                break;
            }
        }

        if ( !allFixed )
        {
            // printf( "NOT removing equation!\n" );
            // equation->dump();

            ++equation;

        }
        else
        {
            double sum = 0;
            for ( const auto &addend : equation->_addends )
                sum += addend._coefficient * _preprocessed.getLowerBound( addend._variable );

            if ( FloatUtils::areDisequal( sum, equation->_scalar, 1e-5 ) )
            {
                printf( "Error! Got sum = %.15lf, scalar = %.15lf, failing...\n",
                        sum,
                        equation->_scalar );

                throw InfeasibleQueryException();
            }

            // printf( "PP: process equations: removing equation\n" );
            // equation->dump();
            // _preprocessed.dump();

            equation = equations.erase( equation );
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

bool Preprocessor::processIdenticalVariables()
{
    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    bool found = false;
    while ( equation != equations.end() )
    {
        // We are only looking for equations of type c(v1 - v2) = 0
        if ( equation->_addends.size() != 2 || equation->_type != Equation::EQ )
        {
            ++equation;
            continue;
        }

        Equation::Addend term1 = equation->_addends.front();
        Equation::Addend term2 = equation->_addends.back();

        if ( FloatUtils::areDisequal( term1._coefficient, -term2._coefficient ) ||
             !FloatUtils::isZero( equation->_scalar ) )
        {
            ++equation;
            continue;
        }

        ASSERT( term1._variable != term2._variable );

        // The equation matches the pattern, process and remove it
        found = true;
        unsigned v1 = term1._variable;
        unsigned v2 = term2._variable;

        double bestLowerBound = FloatUtils::max( _preprocessed.getLowerBound( v1 ),
                                                 _preprocessed.getLowerBound( v2 ) );
        double bestUpperBound = FloatUtils::min( _preprocessed.getUpperBound( v1 ),
                                                 _preprocessed.getUpperBound( v2 ) );

        equation = equations.erase( equation );

        _preprocessed.setLowerBound( v2, bestLowerBound );
        _preprocessed.setUpperBound( v2, bestUpperBound );

        _preprocessed.mergeIdenticalVariables( v1, v2 );

        _mergedVariables[v1] = v2;
    }

    return found;
}

void Preprocessor::eliminateVariables()
{
    // First, collect the variables that have become fixed, and their fixed values
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
            _fixedVariables[i] = _preprocessed.getLowerBound( i );
	}

    // If there's nothing to eliminate, we're done
    if ( _fixedVariables.empty() && _mergedVariables.empty() )
        return;

    if ( _statistics )
        _statistics->ppSetNumEliminatedVars( _fixedVariables.size() + _mergedVariables.size() );

    // Compute the new variable indices, after the elimination of fixed variables
 	int offset = 0;
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( _fixedVariables.exists( i ) || _mergedVariables.exists( i ) )
            ++offset;
        else
            _oldIndexToNewIndex[i] = i - offset;
	}

    // Next, eliminate the fixed variables from the equations
    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    while ( equation != equations.end() )
	{
        // Each equation is of the form sum(addends) = scalar. So, any fixed variable
        // needs to be subtracted from the scalar. Merged variables should have already
        // been removed, so we don't care about them
        List<Equation::Addend>::iterator addend = equation->_addends.begin();
        while ( addend != equation->_addends.end() )
        {
            ASSERT( !_mergedVariables.exists( addend->_variable ) );

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

        // If all the addends have been removed, we remove the entire equation.
        // Overwise, we are done here.
        if ( equation->_addends.empty() )
        {
            if ( _statistics )
                _statistics->ppIncNumEquationsRemoved();

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
            if ( _statistics )
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
        if ( _fixedVariables.exists( i ) || _mergedVariables.exists( i ) )
            continue;

        ASSERT( _oldIndexToNewIndex.at( i ) <= i );

        _preprocessed.setLowerBound( _oldIndexToNewIndex.at( i ), _preprocessed.getLowerBound( i ) );
        _preprocessed.setUpperBound( _oldIndexToNewIndex.at( i ), _preprocessed.getUpperBound( i ) );
	}

    // Make any adjustments in the stored debugging solution, if needed
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
    {
        if ( _preprocessed._debuggingSolution.exists( i ) )
        {
            if ( _fixedVariables.exists( i ) )
            {
                if ( !FloatUtils::areEqual( _fixedVariables[i], _preprocessed._debuggingSolution[i] ) )
                    throw ReluplexError( ReluplexError::DEBUGGING_ERROR,
                                         Stringf( "Variable %u fixed to %.5lf, "
                                                  "contradicts possible solution %.5lf",
                                                  i,
                                                  _fixedVariables[i],
                                                  _preprocessed._debuggingSolution[i] ).ascii() );

                _preprocessed._debuggingSolution.erase( i );
            }
            else if ( _mergedVariables.exists( i ) )
            {
                unsigned oldVar = i;
                unsigned newVar = _mergedVariables[i];

                if ( !_preprocessed._debuggingSolution.exists( newVar ) )
                {
                    _preprocessed._debuggingSolution[newVar] = _preprocessed._debuggingSolution[oldVar];
                }
                else
                {
                    if ( !FloatUtils::areEqual ( _preprocessed._debuggingSolution[newVar],
                                                 _preprocessed._debuggingSolution[oldVar] ) )
                        throw ReluplexError( ReluplexError::DEBUGGING_ERROR,
                                             Stringf( "Variable %u fixed to %.5lf, "
                                                      "merged into %u which was fixed to %.5lf",
                                                      oldVar,
                                                      _mergedVariables[oldVar],
                                                      newVar,
                                                      _mergedVariables[newVar] ).ascii() );

                }

                _preprocessed._debuggingSolution.erase( oldVar );
            }
            else
            {
                _preprocessed._debuggingSolution[_oldIndexToNewIndex[i]] =
                    _preprocessed._debuggingSolution[i];

                if ( _oldIndexToNewIndex[i] != i )
                    _preprocessed._debuggingSolution.erase( i );
            }
        }
    }

    // Adjust the number of variables in the query
    _preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - _fixedVariables.size() - _mergedVariables.size() );
}

bool Preprocessor::variableIsFixed( unsigned index ) const
{
    return _fixedVariables.exists( index );
}

double Preprocessor::getFixedValue( unsigned index ) const
{
    return _fixedVariables.at( index );
}

bool Preprocessor::variableIsMerged( unsigned index ) const
{
    return _mergedVariables.exists( index );
}

unsigned Preprocessor::getMergedIndex( unsigned index ) const
{
    return _mergedVariables.at( index );
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

void Preprocessor::addPlAuxiliaryEquations()
{
    // First, collect all the new equations
    const List<PiecewiseLinearConstraint *> &plConstraints
        ( _preprocessed.getPiecewiseLinearConstraints() );

    List<Equation> newEquations;
    for ( const auto &constraint : plConstraints )
        constraint->getAuxiliaryEquations( newEquations );

    for ( Equation equation : newEquations )
        _preprocessed.addEquation( equation );
}

void Preprocessor::dumpAllBounds( const String &message )
{
    printf( "\nPP: Dumping all bounds (%s)\n", message.ascii() );

    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
    {
        printf( "\tx%u: [%5.2lf, %5.2lf]\n", i, _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) );
    }

    printf( "\n" );
}

void Preprocessor::numericalStability()
{
    // If bounds are almost equal, make them equal (numerical stability issue)
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
    {
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ),
                                   _preprocessed.getUpperBound( i ),
                                   1e-5 ) )
            _preprocessed.setUpperBound( i, _preprocessed.getLowerBound( i ) );
    }


    List<Equation> &equations( _preprocessed.getEquations() );
    List<Equation>::iterator equation = equations.begin();

    while ( equation != equations.end() )
    {
        bool allFixed = true;
        for ( const auto &addend : equation->_addends )
        {
            unsigned var = addend._variable;
            if ( !FloatUtils::areEqual( _preprocessed.getLowerBound( var ),
                                        _preprocessed.getUpperBound( var ) ) )
            {
                allFixed = false;
                break;
            }
        }

        if ( !allFixed )
        {
            ++equation;
        }
        else
        {
            double sum = 0;
            for ( const auto &addend : equation->_addends )
                sum += addend._coefficient * _preprocessed.getLowerBound( addend._variable );

            if ( FloatUtils::areDisequal( sum, equation->_scalar ) )
                throw InfeasibleQueryException();

            // printf( "PP: numerical stability removing equation\n" );
            // equation->dump();

            equation = equations.erase( equation );
        }
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
