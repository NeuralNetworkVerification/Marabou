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
#include "Map.h"
#include "List.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "MString.h"
#include "InputQuery.h"
#include "MString.h"
#include "Set.h"
#include "MStringf.h"
#include "Debug.h"
#include <assert.h>

NetworkParser::NetworkParser()
{
    _numVars = 0;
}

Variable NetworkParser::getNewVariable(){

    _numVars += 1;
    return _numVars-1;
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
}

void NetworkParser::addSigmoid( Variable inputVar, Variable outputVar )
{
    _signList.append( new SignConstraint( inputVar, outputVar ) );
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

void NetworkParser::getMarabouQuery( InputQuery& query )
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

    for ( ReluConstraint* constraintPtr : _reluList )
    {
        ReluConstraint constraint = *constraintPtr;
        ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( SigmoidConstraint* constraintPtr : _sigmoidList )
    {
        SigmoidConstraint constraint = *constraintPtr;
        ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        query.addTranscendentalConstraint( constraintPtr );
    }

    for ( MaxConstraint* constraintPtr : _maxList )
    {
        MaxConstraint constraint = *constraintPtr;
        ASSERT( constraint.getF() < _numVars );
        for ( [[maybe_unused]] Variable var : constraint.getElements() )
        {
            ASSERT ( var < _numVars );
        }
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( AbsoluteValueConstraint* constraintPtr : _absList )
    {
        AbsoluteValueConstraint constraint = *constraintPtr;
        ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    for ( SignConstraint* constraintPtr : _signList )
    {
        SignConstraint constraint = *constraintPtr;
        ASSERT( constraint.getB() < _numVars && constraint.getF() < _numVars );
        query.addPiecewiseLinearConstraint( constraintPtr );
    }

    // TODO check this last two
    for ( std::pair<Variable,float> lower : _lowerBounds )
    {
        ASSERT( lower.first < _numVars );
        query.setLowerBound( lower.first,lower.second );
    }

    for ( std::pair<Variable,float> upper : _upperBounds )
    {
        ASSERT( upper.first < _numVars );
        query.setLowerBound( upper.first,upper.second );
    }
}

int NetworkParser::findEquationWithOutputVariable( Variable variable )
{
    int i = 0;
    for ( Equation& equation : _equationList )
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
