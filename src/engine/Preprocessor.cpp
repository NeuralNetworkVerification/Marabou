/*********************                                                        */
/*! \file Preprocessor.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "Map.h"
#include "Preprocessor.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "Tightening.h"

#ifdef _WIN32
#undef INFINITE
#endif

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

      Then, eliminate fixed variables.
    */

    bool continueTightening = true;
    while ( continueTightening )
    {
        continueTightening = processEquations();
        continueTightening = processConstraints() || continueTightening;
        if ( attemptVariableElimination )
            continueTightening = processIdenticalVariables() || continueTightening;

        if ( _statistics )
            _statistics->ppIncNumTighteningIterations();
    }

    collectFixedValues();
    separateMergedAndFixed();

    if ( attemptVariableElimination )
        eliminateVariables();

    return _preprocessed;
}

void Preprocessor::separateMergedAndFixed()
{
    Map<unsigned, double> noLongerMerged;

    for ( const auto &merged : _mergedVariables )
    {
        // In case of a chained merging, go all the way to the final target
        unsigned finalMergeTarget = merged.second;
        while ( _mergedVariables.exists( finalMergeTarget ) )
            finalMergeTarget = _mergedVariables[finalMergeTarget];

        // Is the merge target fixed?
        if ( _fixedVariables.exists( finalMergeTarget ) )
            noLongerMerged[merged.first] = _fixedVariables[finalMergeTarget];
    }

    // We have collected all the merged variables that should actually be fixed
    for ( const auto &merged : noLongerMerged )
    {
        _mergedVariables.erase( merged.first );
        _fixedVariables[merged.first] = merged.second;
    }

    DEBUG({
            // After this operation, the merged and fixed variable sets are disjoint
            for ( const auto &fixed : _fixedVariables )
                ASSERT( !_mergedVariables.exists( fixed.first ) );
          });
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

            ciSign[xi] = ci > 0 ? POSITIVE : NEGATIVE;

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

                if ( FloatUtils::gt( lowerBound, _preprocessed.getLowerBound( xi ) ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setLowerBound( xi, lowerBound );
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

                if ( FloatUtils::lt( upperBound, _preprocessed.getUpperBound( xi ) ) )
                {
                    tighterBoundFound = true;
                    _preprocessed.setUpperBound( xi, upperBound );
                }
            }

            if ( FloatUtils::gt( _preprocessed.getLowerBound( xi ),
                                 _preprocessed.getUpperBound( xi ),
                                 GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD ) )
            {
                delete[] ciTimesLb;
                delete[] ciTimesUb;
                delete[] ciSign;

                throw InfeasibleQueryException();
            }
        }

        delete[] ciTimesLb;
        delete[] ciTimesUb;
        delete[] ciSign;

        /*
          Next, do another sweep over the equation.
          Look for almost-fixed variables and fix them, and remove the equation
          entirely if it has nothing left to contribute.
        */
        bool allFixed = true;
        for ( const auto &addend : equation->_addends )
        {
            unsigned var = addend._variable;
            double lb = _preprocessed.getLowerBound( var );
            double ub = _preprocessed.getUpperBound( var );

            if ( FloatUtils::areEqual( lb, ub, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD ) )
                _preprocessed.setUpperBound( var, _preprocessed.getLowerBound( var ) );
            else
                allFixed = false;
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

            if ( FloatUtils::areDisequal( sum, equation->_scalar, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD ) )
            {
                throw InfeasibleQueryException();
            }
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
                 ( FloatUtils::gt( tightening._value, _preprocessed.getLowerBound( tightening._variable ) ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setLowerBound( tightening._variable, tightening._value );
            }

            else if ( ( tightening._type == Tightening::UB ) &&
                      ( FloatUtils::lt( tightening._value, _preprocessed.getUpperBound( tightening._variable ) ) ) )
            {
                tighterBoundFound = true;
                _preprocessed.setUpperBound( tightening._variable, tightening._value );
            }

            if ( FloatUtils::areEqual( _preprocessed.getLowerBound( tightening._variable ),
                                       _preprocessed.getUpperBound( tightening._variable ),
                                       GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD ) )
                _preprocessed.setUpperBound( tightening._variable,
                                             _preprocessed.getLowerBound( tightening._variable ) );
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

        double bestLowerBound =
            _preprocessed.getLowerBound( v1 ) > _preprocessed.getLowerBound( v2 ) ?
            _preprocessed.getLowerBound( v1 ) :
            _preprocessed.getLowerBound( v2 );

        double bestUpperBound =
            _preprocessed.getUpperBound( v1 ) < _preprocessed.getUpperBound( v2 ) ?
            _preprocessed.getUpperBound( v1 ) :
            _preprocessed.getUpperBound( v2 );

        equation = equations.erase( equation );

        _preprocessed.setLowerBound( v2, bestLowerBound );
        _preprocessed.setUpperBound( v2, bestUpperBound );

        _preprocessed.mergeIdenticalVariables( v1, v2 );

        _mergedVariables[v1] = v2;
    }

    return found;
}

void Preprocessor::collectFixedValues()
{
    // Compute all used variables:
    //   1. Variables that appear in equations
    //   2. Variables that participate in PL constraints
    //   3. Variables that have been merged (and hence, previously
    //      appeared in an equation)
    Set<unsigned> usedVariables;
    for ( const auto &equation : _preprocessed.getEquations() )
        usedVariables += equation.getParticipatingVariables();
    for ( const auto &constraint : _preprocessed.getPiecewiseLinearConstraints() )
    {
        for ( const auto &var : constraint->getParticipatingVariables() )
            usedVariables.insert( var );
    }
    for ( const auto &merged : _mergedVariables )
        usedVariables.insert( merged.first );

    // Collect any variables with identical lower and upper bounds, or
    // which are unused
	for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( FloatUtils::areEqual( _preprocessed.getLowerBound( i ), _preprocessed.getUpperBound( i ) ) )
        {
            _fixedVariables[i] = _preprocessed.getLowerBound( i );
        }
        else if ( !usedVariables.exists( i ) )
        {
            // If possible, choose a value that matches the debugging
            // solution. Otherwise, pick the lower bound
            if ( _preprocessed._debuggingSolution.exists( i ) &&
                 _preprocessed._debuggingSolution[i] >= _preprocessed.getLowerBound( i ) &&
                 _preprocessed._debuggingSolution[i] <= _preprocessed.getUpperBound( i ) )
            {
                _fixedVariables[i] = _preprocessed._debuggingSolution[i];
            }
            else
            {
                _fixedVariables[i] = _preprocessed.getLowerBound( i );
            }
        }
	}
}

void Preprocessor::eliminateVariables()
{
    // If there's nothing to eliminate, we're done
    if ( _fixedVariables.empty() && _mergedVariables.empty() )
        return;

    if ( _statistics )
        _statistics->ppSetNumEliminatedVars( _fixedVariables.size() + _mergedVariables.size() );

    // Check and remove any fixed variables from the debugging solution
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
    {
        if ( _fixedVariables.exists( i ) && _preprocessed._debuggingSolution.exists( i ) )
        {
            if ( !FloatUtils::areEqual( _fixedVariables[i], _preprocessed._debuggingSolution[i] ) )
                throw MarabouError( MarabouError::DEBUGGING_ERROR,
                                     Stringf( "Variable %u fixed to %.5lf, "
                                              "contradicts possible solution %.5lf",
                                              i,
                                              _fixedVariables[i],
                                              _preprocessed._debuggingSolution[i] ).ascii() );

            _preprocessed._debuggingSolution.erase( i );
        }
    }

    // Check and remove any merged variables from the debugging solution
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
    {
        if ( _mergedVariables.exists( i ) && _preprocessed._debuggingSolution.exists( i ) )
        {
            double newVar = _mergedVariables[i];

            if ( _preprocessed._debuggingSolution.exists( newVar ) )
            {

                if ( !FloatUtils::areEqual ( _preprocessed._debuggingSolution[i],
                                             _preprocessed._debuggingSolution[newVar] ) )
                    throw MarabouError( MarabouError::DEBUGGING_ERROR,
                                         Stringf( "Variable %u fixed to %.5lf, "
                                                  "merged into %u which was fixed to %.5lf",
                                                  i,
                                                  _preprocessed._debuggingSolution[i],
                                                  newVar,
                                                  _preprocessed._debuggingSolution[newVar] ).ascii() );
            }
            else
            {
                _preprocessed._debuggingSolution[newVar] = _preprocessed._debuggingSolution[i];
            }

            _preprocessed._debuggingSolution.erase( i );
        }
    }

    // Inform the NLR about eliminated varibales
    if ( _preprocessed._networkLevelReasoner )
    {
        for ( const auto &fixed : _fixedVariables )
            _preprocessed._networkLevelReasoner->eliminateVariable( fixed.first, fixed.second );
    }

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

            delete *constraint;
            *constraint = NULL;
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

    // Let the NLR know of changes in indices and merged variables
    if ( _preprocessed._networkLevelReasoner )
        _preprocessed._networkLevelReasoner->updateVariableIndices( _oldIndexToNewIndex, _mergedVariables );

    // Update the lower/upper bound maps
    for ( unsigned i = 0; i < _preprocessed.getNumberOfVariables(); ++i )
	{
        if ( _fixedVariables.exists( i ) || _mergedVariables.exists( i ) )
            continue;

        ASSERT( _oldIndexToNewIndex.at( i ) <= i );

        _preprocessed.setLowerBound( _oldIndexToNewIndex.at( i ), _preprocessed.getLowerBound( i ) );
        _preprocessed.setUpperBound( _oldIndexToNewIndex.at( i ), _preprocessed.getUpperBound( i ) );
	}

    // Adjust variable indices in the debugging solution
    Map<unsigned, double> copy = _preprocessed._debuggingSolution;
    _preprocessed._debuggingSolution.clear();
    for ( const auto &debugPair : copy )
    {
        unsigned variable = debugPair.first;
        double value = debugPair.second;

        // Go through any merges
        while ( variableIsMerged( variable ) )
            variable = getMergedIndex( variable );

        // Grab new index
        variable = getNewIndex( variable );

        _preprocessed._debuggingSolution[variable] = value;
    }

    // Adjust the number of variables in the query
    _preprocessed.setNumberOfVariables( _preprocessed.getNumberOfVariables() - _fixedVariables.size() - _mergedVariables.size() );

    // Adjust the input/output mappings in the query
    _preprocessed.adjustInputOutputMapping( _oldIndexToNewIndex, _mergedVariables );
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
    const List<PiecewiseLinearConstraint *> &plConstraints
        ( _preprocessed.getPiecewiseLinearConstraints() );

    for ( const auto &constraint : plConstraints )
        constraint->addAuxiliaryEquations( _preprocessed );
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
