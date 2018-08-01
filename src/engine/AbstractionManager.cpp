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

        break;
    }

    return result;
}

void AbstractionManager::storeOriginalQuery( InputQuery &inputQuery )
{
    _originalQuery = inputQuery;

    for ( const auto &plConstraint : _originalQuery.getPiecewiseLinearConstraints() )
    {
        List<unsigned> variables = plConstraint->getParticipatingVariables();
        for ( unsigned variable : variables )
        {
            plConstraint->notifyLowerBound( variable, _originalQuery.getLowerBound( variable ) );
            plConstraint->notifyUpperBound( variable, _originalQuery.getUpperBound( variable ) );
        }
    }

    _originalQuery = Preprocessor().preprocess( _originalQuery, true );

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
    _ppAbstractQuery = _abstractQuery;

    for ( const auto &plConstraint : _ppAbstractQuery.getPiecewiseLinearConstraints() )
    {
        List<unsigned> variables = plConstraint->getParticipatingVariables();
        for ( unsigned variable : variables )
        {
            plConstraint->notifyLowerBound( variable, _ppAbstractQuery.getLowerBound( variable ) );
            plConstraint->notifyUpperBound( variable, _ppAbstractQuery.getUpperBound( variable ) );
        }
    }

    _ppAbstractQuery = Preprocessor().preprocess( _ppAbstractQuery, true );

    if ( _ppAbstractQuery.countInfiniteBounds() != 0 )
    {
        printf( "Error! preprocessed abstract query has infinite bounds\n" );
        exit ( 1 );
    }

    Engine engine;
    if ( !engine.processInputQuery( _ppAbstractQuery ) )
        return false;

    if ( !engine.solve() )
        return false;

    engine.extractSolution( _ppAbstractQuery );

    return true;
}

void AbstractionManager::extractSatAssignment()
{
}

bool AbstractionManager::spurious()
{
    InputQuery copy = _originalQuery;

    // Fix the input variables to their solution values
    for ( const auto &input : _originalQuery.getInputVariables() )
    {
        copy.setLowerBound( input, _ppAbstractQuery.getSolutionValue( input ) );
        copy.setUpperBound( input, _ppAbstractQuery.getSolutionValue( input ) );
    }

    // Use the preprocessor to propagate these values through the network
    copy = Preprocessor().preprocess( copy, false );

    // Make sure that a value has been calculated for every variable
    for ( unsigned i = 0; i < copy.getNumberOfVariables(); ++i )
    {
        if ( !FloatUtils::areEqual( copy.getLowerBound( i ), copy.getUpperBound( i ) ) )
        {
            printf( "Error! Could not calculate an exact assignment!\n" );
            exit( 1 );
        }
    }

    // Go over the variables and see if everything matches
    for ( unsigned i = 0; i < copy.getNumberOfVariables(); ++i )
    {
        if ( !FloatUtils::areEqual( copy.getLowerBound( i ), _ppAbstractQuery.getSolutionValue( i ) ) )
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
