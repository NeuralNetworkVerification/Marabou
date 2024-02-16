/*********************                                                        */
/*! \file NetworkParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt, Luca Arnaboldi
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This file provides a general interface for parsing a neural network file.
 ** Keeps track of internal state such as equations and variables that
 ** may be altered during parsing of a network. Once the network has been parsed
 ** they are then loaded into an InputQuery.
 ** Future parsers for individual network formats should extend this interface.
 **/

#include "NetworkParser.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "List.h"
#include "MString.h"
#include "MStringf.h"
#include "Map.h"
#include "Set.h"

#include <assert.h>

NetworkParser::NetworkParser()
{
    _numVars = 0;
}

Variable NetworkParser::getNewVariable()
{
    _numVars += 1;
    return _numVars - 1;
}

void NetworkParser::addEquation( Equation &eq )
{
    _equationList.append( eq );
}

void NetworkParser::setLowerBound( Variable var, float value )
{
    _lowerBounds[var] = value;
}

void NetworkParser::setUpperBound( Variable var, float value )
{
    _upperBounds[var] = value;
}

void NetworkParser::addRelu( Variable inputVar, Variable outputVar )
{
    _reluList.append( new ReluConstraint( inputVar, outputVar ) );
    setLowerBound( outputVar, 0.0f );
}

void NetworkParser::addLeakyRelu( Variable inputVar, Variable outputVar, float alpha )
{
    _leakyReluList.append( new LeakyReluConstraint( inputVar, outputVar, alpha ) );
}

void NetworkParser::addSigmoid( Variable inputVar, Variable outputVar )
{
    _sigmoidList.append( new SigmoidConstraint( inputVar, outputVar ) );
    setLowerBound( outputVar, 0.0 );
    setUpperBound( outputVar, 1.0 );
}

void NetworkParser::addTanh( Variable inputVar, Variable outputVar )
{
    // Uses the identity `tanh(x) = 2 * sigmoid(2x) - 1` to implement
    // it terms of a sigmoid constraint.
    Variable firstAffine = getNewVariable();
    Variable sigmoidOutput = getNewVariable();

    Equation e1;
    e1.addAddend( 2.0, inputVar );
    e1.addAddend( -1.0, firstAffine );
    e1.setScalar( 0.0 );

    Equation e2;
    e2.addAddend( 2.0, sigmoidOutput );
    e2.addAddend( -1.0, outputVar );
    e2.setScalar( 1.0 );

    addEquation( e1 );
    addSigmoid( firstAffine, sigmoidOutput );
    addEquation( e2 );
    setLowerBound( outputVar, -1.0 );
    setUpperBound( outputVar, 1.0 );
}

void NetworkParser::addMaxConstraint( Variable var, Set<Variable> elements )
{
    _maxList.append( new MaxConstraint( var, elements ) );
}

void NetworkParser::addSignConstraint( Variable inputVar, Variable outputVar )
{
    _signList.append( new SignConstraint( inputVar, outputVar ) );
}

void NetworkParser::addAbsConstraint( Variable inputVar, Variable outputVar )
{
    _absList.append( new AbsoluteValueConstraint( inputVar, outputVar ) );
}

void NetworkParser::getMarabouQuery( InputQuery &query )
{
    query.setNumberOfVariables( _numVars );

    int i = 0;
    for ( Variable inputVar : _inputVars )
    {
        query.markInputVariable( inputVar, i );
        i++;
    }

    int j = 0;
    for ( Variable outputVar : _outputVars )
    {
        query.markOutputVariable( outputVar, j );
        j++;
    }

    for ( Equation equation : _equationList )
    {
        query.addEquation( equation );
    }

    for ( ReluConstraint *constraintPtr : _reluList )
    {
        DEBUG( {
            ReluConstraint constraint = *constraintPtr;
            ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        } );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( LeakyReluConstraint *constraintPtr : _leakyReluList )
    {
        DEBUG( {
            LeakyReluConstraint constraint = *constraintPtr;
            ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        } );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( SigmoidConstraint *constraintPtr : _sigmoidList )
    {
        DEBUG( {
            SigmoidConstraint constraint = *constraintPtr;
            ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        } );
        query.addNonlinearConstraint( constraintPtr );
    }

    for ( MaxConstraint *constraintPtr : _maxList )
    {
        DEBUG( {
            MaxConstraint constraint = *constraintPtr;
            ASSERT( constraint.getF() < _numVars );
            for ( [[maybe_unused]] Variable var : constraint.getElements() )
            {
                ASSERT( var < _numVars );
            }
        } );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( AbsoluteValueConstraint *constraintPtr : _absList )
    {
        DEBUG( {
            AbsoluteValueConstraint constraint = *constraintPtr;
            ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        } );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( SignConstraint *constraintPtr : _signList )
    {
        DEBUG( {
            SignConstraint constraint = *constraintPtr;
            ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        } );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    // TODO check this last two
    for ( std::pair<Variable, float> lower : _lowerBounds )
    {
        ASSERT( lower.first < _numVars );
        query.setLowerBound( lower.first, lower.second );
    }

    for ( std::pair<Variable, float> upper : _upperBounds )
    {
        ASSERT( upper.first < _numVars );
        query.setUpperBound( upper.first, upper.second );
    }
}

int NetworkParser::findEquationWithOutputVariable( Variable variable )
{
    int i = 0;
    for ( Equation &equation : _equationList )
    {
        Equation::Addend outputAddend = equation._addends.back();
        if ( variable == outputAddend._variable )
        {
            ASSERT( outputAddend._coefficient == -1 );
            return i;
        }
        i++;
    }
    return -1;
}
