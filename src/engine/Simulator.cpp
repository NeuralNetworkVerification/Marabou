/*********************                                                        */
/*! \file Simulator.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "Simulator.h"

void Simulator::runSimulations( const InputQuery &inputQuery, unsigned numberOfSimulations, unsigned seed )
{
    // Store the original query, so a fresh copy can be used in every simulation
    storeOriginalQuery( inputQuery );

    // Initialize randomness
    if ( seed == 0 )
        seed = time( NULL );
    srand( seed );

    // Perform the actual simulations
    for ( unsigned i = 0; i < numberOfSimulations; ++i )
        runSingleSimulation();
}

void Simulator::storeOriginalQuery( const InputQuery &inputQuery )
{
    _originalQuery = inputQuery;

    // for ( const auto &plConstraint : _originalQuery.getPiecewiseLinearConstraints() )
    // {
    //     List<unsigned> variables = plConstraint->getParticipatingVariables();
    //     for ( unsigned variable : variables )
    //     {
    //         plConstraint->notifyLowerBound( variable, _originalQuery.getLowerBound( variable ) );
    //         plConstraint->notifyUpperBound( variable, _originalQuery.getUpperBound( variable ) );
    //     }
    // }

    // _originalQuery = Preprocessor().preprocess( _originalQuery, false );

    if ( _originalQuery.countInfiniteBounds() != 0 )
        throw ReluplexError( ReluplexError::SIMULATOR_ERROR, "Preprocessed query has infinite bounds" );

    if ( _originalQuery.getNumInputVariables() == 0 )
        throw ReluplexError( ReluplexError::SIMULATOR_ERROR, "Preprocessed query has no input variables" );
}

void Simulator::runSingleSimulation()
{
    InputQuery query = _originalQuery;

    List<unsigned> inputs = query.getInputVariables();
    for ( const auto &input : inputs )
    {
        double lb = query.getLowerBound( input );
        double ub = query.getUpperBound( input );

        double factor = ((double)rand()) / RAND_MAX;
        double value = lb + factor * ( ub - lb );

        query.setLowerBound( input, value );
        query.setUpperBound( input, value );
    }

    // Propagate the random values through the network using the preprocessor
    Preprocessor preprocessor;

    // If we set elimination to true, the PL constraints will be removed
    query = preprocessor.preprocess( query, false );

    // Extract the result
    Simulator::Result result;
    for ( unsigned i = 0; i < _originalQuery.getNumberOfVariables(); ++i )
    {
        // Make sure that a value has been calculated for every variable
        ASSERT( !preprocessor.variableIsMerged( i ) );
        if ( !preprocessor.variableIsFixed( i ) )
            throw ReluplexError( ReluplexError::SIMULATOR_ERROR, "Could not calculate an exact assignment" );

        ASSERT( FloatUtils::areEqual( query.getLowerBound( i ),
                                      query.getUpperBound( i ) ) );

        result[i] = query.getLowerBound( i );
    }

    _results.append( result );
}

const List<Simulator::Result> *Simulator::getResults()
{
    return &_results;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
