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
#include "InputQuery.h"
#include "MString.h"
#include "Preprocessor.h"
#include "ReluConstraint.h"
#include "Simulator.h"

#include <cstdlib>
#include <ctime>

void Simulator::processInputQuery( const InputQuery &inputQuery )
{
    srand( time( NULL ) );

    if ( inputQuery.countInfiniteBounds() != 0 )
    {
        log( "Have variables with infinite bounds, not doing anything" );
        return;
    }

    if ( inputQuery.getInputVariables().empty() )
    {
        log( "No variables marked as input variables, not doing anything" );
        return;
    }

    _originalQuery = inputQuery;

    enum
    {
        NUMBER_OF_SIMULATIONS = 100,
    };

    _signatures.clear();
    for ( unsigned i = 0; i < NUMBER_OF_SIMULATIONS; ++i )
        collectOneSignature();
}

void Simulator::collectOneSignature()
{
    // Pick a random value for each of the input variables
    InputQuery copy = _originalQuery;

    List<unsigned> inputs = copy.getInputVariables();
    for ( const auto &input : inputs )
    {
        double lb = copy.getLowerBound( input );
        double ub = copy.getUpperBound( input );

        double factor = ((double)rand()) / RAND_MAX;
        double value = lb + factor * ( ub - lb );

        copy.setLowerBound( input, value );
        copy.setUpperBound( input, value );
    }

    // Propagate random values through the network
    Preprocessor preprocessor;
    preprocessor.preprocess( copy, true );

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

    // Exract the signature
    ActivationSignature signature;
    for ( const auto &constraint : copy.getPiecewiseLinearConstraints() )
    {
        ReluConstraint *relu = (ReluConstraint *)constraint;
        ASSERT( relu->phaseFixed() );
        signature._signature.append( relu->phaseFixedToActive() );
    }
}

void Simulator::log( const String &message ) const
{
    printf( "Simulator: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
