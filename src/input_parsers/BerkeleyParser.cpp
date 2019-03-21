/*********************                                                        */
/*! \file BerkeleyParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

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

    unsigned numberOfVariables =
        _berkeleyNeuralNetwork.getNumVariables();

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
    unsigned constraintID = 1;
    Map<unsigned, unsigned> fToB = _berkeleyNeuralNetwork.getFToB();
    for ( const auto &it : fToB )
    {
        unsigned f = it.first;
        unsigned b = it.second;

        PiecewiseLinearConstraint *relu = new ReluConstraint( b, f, constraintID++ );
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

Set<unsigned> BerkeleyParser::getOutputVariables() const
{
    return _berkeleyNeuralNetwork.getOutputVariables();
}

void BerkeleyParser::addAuxiliaryEquations( List<Equation> &auxiliaryEquations )
{
    // Go over the F variables, and look for inner layers
    for ( const auto &fToB : _berkeleyNeuralNetwork.getFToB() )
        addAuxiliaryEquation( fToB.first, fToB.second, auxiliaryEquations );
}

void BerkeleyParser::addAuxiliaryEquation( unsigned xf, unsigned xb, List<Equation> &auxiliaryEquations )
{
    Set<unsigned> inputVariables = _berkeleyNeuralNetwork.getInputVariables();

    // Find the equation for xb
    for ( const auto &eq : _berkeleyNeuralNetwork.getEquations() )
    {
        if ( eq._lhs == xb )
        {
            /*
              xb is given as: xb = sum(ci xi) + bias.

              We can assume that xi's are non negative if they are input variables,
              in which case they are in the range [0,1], or if they are the
              outputs of previous ReLUs.

              We construct the equation:

              xf = sum(cj xj) + max(bias, 0)

              where cj are those ci coefficients that are non-negative.
            */

            Equation equation;
            for ( const auto &addend : eq._rhs )
            {
                if ( ( _berkeleyNeuralNetwork.getInputVariables().exists( addend._var ) ) ||
                     ( _berkeleyNeuralNetwork.getFToB().exists( addend._var ) ) )
                {
                    if ( FloatUtils::isPositive( addend._coefficient ) )
                        equation.addAddend( addend._coefficient, addend._var );
                }
            }

            // If we couldn't find any viable addends, skips
            if ( equation._addends.empty() )
                return;

            equation.addAddend( -1.0, xf );

            if ( FloatUtils::isPositive( eq._constant ) )
                equation.setScalar( eq._constant );
            else
                equation.setScalar( 0.0 );

            auxiliaryEquations.append( equation );
            return;
        }
    }

    printf( "Error!! Couldn't find an equation for one of the B's\n" );
    exit( 1 );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
