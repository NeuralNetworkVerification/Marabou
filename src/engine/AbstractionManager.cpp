/*********************                                                        */
/*! \file AbstractionManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "AbstractionManager.h"
#include "Debug.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "ReluConstraint.h"

bool AbstractionManager::run( InputQuery &inputQuery )
{
    storeOriginalQuery( inputQuery );
    createInitialAbstraction();

    bool result = false;
    while ( true )
    {
        log( "Main loop starting" );

        printf( "Number of restored constraints: %u / %u\n",
                _abstractQuery.getPiecewiseLinearConstraints().size(),
                _originalQuery.getPiecewiseLinearConstraints().size() );

        // Run the solver
        result = checkSatisfiability();

        if ( !result )
        {
            // If the result is UNSAT, we are done
            log( "checkSatisfiability returned UNSAT, we are done" );
            break;
        }
        else
        {
            if ( !spurious() )
            {
                // A satisfying assignment
                log( "checkSatisfiability returned SAT, and the assignment is valid - we are done " );

                printf( "Dumping abstract query\n" );
                _abstractQuery.dump();
                _abstractQuery.dumpSolution();

                extractSatAssignment();
                break;
            }
            else
            {
                // We have a spurious SAT
                log( "checkSatisfiability returned SAT, but it is spurious. Refining...\n" );
                refineAbstraction();
            }
        }
    }

    return result;
}

void AbstractionManager::storeOriginalQuery( InputQuery &inputQuery )
{
    _originalQuery = inputQuery;

    printf( "Dumping original query\n" );
    _originalQuery.dump();

    for ( const auto &plConstraint : _originalQuery.getPiecewiseLinearConstraints() )
    {
        List<unsigned> variables = plConstraint->getParticipatingVariables();
        for ( unsigned variable : variables )
        {
            plConstraint->notifyLowerBound( variable, _originalQuery.getLowerBound( variable ) );
            plConstraint->notifyUpperBound( variable, _originalQuery.getUpperBound( variable ) );
        }
    }

    _originalQuery = Preprocessor().preprocess( _originalQuery, false );

    if ( _originalQuery.countInfiniteBounds() != 0 )
    {
        printf( "Error! preprocessed original query has infinite bounds\n" );
        exit ( 1 );
    }
}

void AbstractionManager::createInitialAbstraction()
{
    _abstractQuery.clear();

    // Equations and bounds are copied as is
    _abstractQuery.setNumberOfVariables( _originalQuery.getNumberOfVariables() );
    _abstractQuery.copyEquatiosnAndBounds( _originalQuery );

    // Replace all f = ReLU(b) with f >= b
    for ( const auto &constraint : _originalQuery.getPiecewiseLinearConstraints() )
    {
        // For now assume these are all ReLUs
        ReluConstraint *relu = (ReluConstraint *)constraint;

        // For the case ReLU, the equation replacing it is precisely the aux equation
        List<Equation> auxEquations;
        relu->getAuxiliaryEquations( auxEquations );

        if ( auxEquations.size() != 1U )
        {
            printf( "Error! More than one aux equation!\n" );
            exit( 1 );
        }

        _abstractQuery.addEquation( *auxEquations.begin() );
    }
}

bool AbstractionManager::checkSatisfiability()
{
    Engine engine;
    if ( !engine.processInputQuery( _abstractQuery ) )
        return false;

    if ( !engine.solve() )
        return false;

    engine.extractSolution( _abstractQuery );

    return true;
}

void AbstractionManager::extractSatAssignment()
{
    printf( "extractSatAssignment not yet supported!\n" );
    exit( 1 );
}

bool AbstractionManager::spurious()
{
    _copyOfOriginalQuery.clear();
    _copyOfOriginalQuery = _originalQuery;

    // Fix the input variables to their solution values
    for ( const auto &input : _originalQuery.getInputVariables() )
    {
        _copyOfOriginalQuery.setLowerBound( input, _abstractQuery.getSolutionValue( input ) );
        _copyOfOriginalQuery.setUpperBound( input, _abstractQuery.getSolutionValue( input ) );
    }

    // Use the preprocessor to propagate these values through the network
    Preprocessor preprocessor;
    _copyOfOriginalQuery = preprocessor.preprocess( _copyOfOriginalQuery, true );
    _copyOfOriginalQuery.dump();

    // Make sure that a value has been calculated for every variable
    for ( unsigned i = 0; i < _originalQuery.getNumberOfVariables(); ++i )
    {
        ASSERT( !preprocessor.variableIsMerged( i ) );

        if ( !preprocessor.variableIsFixed( i ) )
        {
            printf( "Error! Could not calculate an exact assignment!\n" );
            exit( 1 );
        }
    }

    // Go over the variables and see if everything matches
    for ( unsigned i = 0; i < _originalQuery.getNumberOfVariables(); ++i )
    {
        double value = preprocessor.getFixedValue( i );
        if ( !FloatUtils::areEqual( value, _abstractQuery.getSolutionValue( i ) ) )
        {
            log( "assignment is spurious!" );
            return true;
        }
    }

    log( "assignment is NOT spurious!" );
    return false;
}

void AbstractionManager::refineAbstraction()
{
    // At least one of the original PL constraints does not hold.
    for ( const auto &constraint : _originalQuery.getPiecewiseLinearConstraints() )
    {
        PiecewiseLinearConstraint *dup = constraint->duplicateConstraint();
        List<unsigned> vars = dup->getParticipatingVariables();

        for ( const auto var : vars )
            dup->notifyVariableValue( var, _abstractQuery.getSolutionValue( var ) );

        if ( !dup->satisfied() )
        {
            // Add the violated constraint to the abstract query, and preprocess it again
            _abstractQuery.addPiecewiseLinearConstraint( constraint->duplicateConstraint() );
            return;
        }
    }
}

void AbstractionManager::log( const String &message ) const
{
    printf( "AbstractionManager: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
