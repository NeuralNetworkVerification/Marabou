/*********************                                                        */
/*! \file BerkeleyParser.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "BerkeleyParser.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "ReluConstraint.h"

BerkeleyParser::BerkeleyParser( const String &path )
    : _berkeleyNeuralNetwork( path )
{
}

void BerkeleyParser::generateQuery( InputQuery &inputQuery )
{
    _berkeleyNeuralNetwork.parseFile();

    // The total number of variables required for the encoding is computed as follows:
    //   1. One for every variable that's part of the query
    //   2. One auxiliary variable for every equation in the query

    unsigned numberOfVariables =
        _berkeleyNeuralNetwork.getNumVariables() +
        _berkeleyNeuralNetwork.getNumEquations();

    printf( "Total number of Marabou variables: %u\n", numberOfVariables );

    inputQuery.setNumberOfVariables( numberOfVariables );

    // Set bounds for inputs. Currently just [0,1]
    Set<unsigned> inputVariables = _berkeleyNeuralNetwork.getInputVariables();
    for ( const auto &it : inputVariables )
    {
        double min = 0.0;
        double max = 1.0;

        inputQuery.setLowerBound( it, min );
        inputQuery.setUpperBound( it, max );
    }

    // Declare relu pairs and set bounds
    Map<unsigned, unsigned> fToB = _berkeleyNeuralNetwork.getFToB();
    for ( const auto &it : fToB )
    {
        unsigned f = it.first;
        unsigned b = it.second;

        PiecewiseLinearConstraint *relu = new ReluConstraint( b, f );
        inputQuery.addPiecewiseLinearConstraint( relu );

        inputQuery.setLowerBound( f, 0.0 );
        inputQuery.setUpperBound( f, FloatUtils::infinity() );

        inputQuery.setLowerBound( b, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( b, FloatUtils::infinity() );
    }

    // Create the equations
    List<BerkeleyNeuralNetwork::Equation> equations = _berkeleyNeuralNetwork.getEquations();
    for ( const auto &berkeleyEquation : equations )
    {
        Equation marabouEquation;

        unsigned auxVar = berkeleyEquation._index + _berkeleyNeuralNetwork.getNumVariables();
        marabouEquation.markAuxiliaryVariable( auxVar );
        marabouEquation.addAddend( 1, auxVar );

        // Set the bounds for all auxiliary variables to 0
        inputQuery.setLowerBound( auxVar, 0.0 );
        inputQuery.setUpperBound( auxVar, 0.0 );

        // The Berkeley equation is of the form y = x1 + x2 + x3 + c.
        // The Marabou equation is of the form y - x1 - x2 - x3 = c.
        unsigned lhs = berkeleyEquation._lhs;
        marabouEquation.addAddend( 1.0, lhs );

        for ( const auto &rhs : berkeleyEquation._rhs )
        {
            unsigned var = rhs._var;
            double coefficient = rhs._coefficient;
            marabouEquation.addAddend( -coefficient, var );
        }

        marabouEquation.setScalar( berkeleyEquation._constant );
        inputQuery.addEquation( marabouEquation );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
