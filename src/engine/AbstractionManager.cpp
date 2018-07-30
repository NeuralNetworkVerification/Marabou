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

#include "FloatUtils.h"
#include "AbstractionManager.h"
#include "Engine.h"
#include "InputQuery.h"

bool AbstractionManager::run( InputQuery &inputQuery )
{
    _originalQuery = inputQuery;
    storeOriginalQuery( inputQuery );

    createInitialAbstraction();

    bool result = false;
    while ( true )
    {
        // Run the solver
        bool result = checkSatisfiability();

        if ( !result )
        {
            // If the result is UNSAT, we are done
            break;
        }
        else
        {
            if ( !spurious() )
            {
                // A satisfying assignment
                break;
            }
            else
            {
                // We have a spurious SAT
                refineAbstraction();
            }
        }
    }

    return result;
}

void AbstractionManager::storeOriginalQuery( InputQuery &inputQuery )
{
    // Store and preprocess the input query
    _originalQuery = inputQuery;

    for ( const auto &plConstraint : _originalQuery.getPiecewiseLinearConstraints() )
    {
        List<unsigned> variables = plConstraint->getParticipatingVariables();
        for ( unsigned variable : variables )
        {
            plConstraint->notifyLowerBound( variable, inputQuery.getLowerBound( variable ) );
            plConstraint->notifyUpperBound( variable, inputQuery.getUpperBound( variable ) );
        }
    }

    _originalQuery = Preprocessor().preprocess( _originalQuery, true );

    if ( _originalQuery.countInfiniteBounds() != 0 )
    {
        printf( "Error! storeOriginalQuery found infinite bounds\n" );
        exit ( 1 );
    }
}

void AbstractionManager::createInitialAbstraction()
{
    _abstractQuery = _originalQuery;
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

bool AbstractionManager::spurious()
{
    InputQuery copy = _originalQuery;

    // Fix the input variables to their solution values
    for ( const auto &input : _originalQuery.getInputVariables() )
    {
        copy.setLowerBound( input, _abstractQuery.getSolutionValue( input ) );
        copy.setUpperBound( input, _abstractQuery.getSolutionValue( input ) );
    }

    // Use the preprocessor to propagate these values through the network
    copy = Preprocessor().preprocess( copy, true );

    // Go over the variables and see if everything matches
    for ( unsigned i = 0; i < copy.getNumberOfVariables(); ++i )
    {
        if ( !FloatUtils::areEqual( copy.getLowerBound( i ), _abstractQuery.getSolutionValue( i ) ) )
            return false;
    }

    return true;
}

void AbstractionManager::refineAbstraction()
{
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
