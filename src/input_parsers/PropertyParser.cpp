/*********************                                                        */
/*! \file PropertyParser.cpp
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

#include "Debug.h"
#include "File.h"
#include "InputParserError.h"
#include "MStringf.h"
#include "PropertyParser.h"
#include <regex>

static bool isScalar( const String &token )
{
    const std::regex floatRegex( "[-+]?[0-9]*\\.?[0-9]+" );
    return std::regex_match( token.ascii(), floatRegex );
}

static double extractScalar( const String &token )
{
    return atof( token.ascii() );
}

void PropertyParser::parse( const String &propertyFilePath, InputQuery &inputQuery )
{
    if ( !File::exists( propertyFilePath ) )
    {
        printf( "Error: the specified property file (%s) doesn't exist!\n", propertyFilePath.ascii() );
        throw InputParserError( InputParserError::FILE_DOESNT_EXIST, propertyFilePath.ascii() );
    }

    File propertyFile( propertyFilePath );
    propertyFile.open( File::MODE_READ );

    try
    {
        while ( true )
        {
            String line = propertyFile.readLine().trim();
            processSingleLine( line, inputQuery );
        }
    }
    catch ( const CommonError &e )
    {
        // A "READ_FAILED" is how we know we're out of lines
        if ( e.getCode() != CommonError::READ_FAILED )
            throw e;
    }
}

void PropertyParser::processSingleLine( const String &line, InputQuery &inputQuery )
{
    List<String> tokens = line.tokenize( " " );

    if ( tokens.size() < 3 )
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, line.ascii() );

    auto it = tokens.rbegin();
    if ( !isScalar( *it ) )
    {
        Stringf message( "Right handside must be scalar in the line: %s", line.ascii() );
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, message.ascii() );
    }

    double scalar = extractScalar( *it );
    ++it;
    Equation::EquationType type = extractRelationSymbol( *it );
    ++it;

    // Now extract the addends. In the special case where we only have
    // one addend, we add this equation as a bound. Otherwise, we add
    // as an equation.
    if ( tokens.size() == 3 )
    {
        // Special case: add as a bound
        String token = (*it).trim();

        bool inputVariable = token.contains( "x" );
        bool outputVariable = token.contains( "y" );
        bool hiddenVariable = token.contains( "h" );

        // Make sure that we have identified precisely one kind of variable
        unsigned variableKindSanity = 0;
        if ( inputVariable ) ++variableKindSanity;
        if ( outputVariable ) ++variableKindSanity;
        if ( hiddenVariable ) ++variableKindSanity;

        if ( variableKindSanity != 1 )
            throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

        // Determine the index (in input query terms) of the variable whose
        // bound is being set.

        unsigned variable = 0;
        List<String> subTokens;

        if ( inputVariable )
        {
            subTokens = token.tokenize( "x" );

            if ( subTokens.size() != 1 )
                throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

            unsigned justIndex = atoi( subTokens.rbegin()->ascii() );

            ASSERT( justIndex < inputQuery.getNumInputVariables() );
            variable = inputQuery.inputVariableByIndex( justIndex );
        }
        else if ( outputVariable )
        {
            subTokens = token.tokenize( "y" );

            if ( subTokens.size() != 1 )
                throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

            unsigned justIndex = atoi( subTokens.rbegin()->ascii() );

            ASSERT( justIndex < inputQuery.getNumOutputVariables() );
            variable = inputQuery.outputVariableByIndex( justIndex );
        }
        else if ( hiddenVariable )
        {
            // These variables are of the form h_2_5
            subTokens = token.tokenize( "_" );

            if ( subTokens.size() != 3 )
                throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

            auto subToken = subTokens.begin();
            ++subToken;
            unsigned layerIndex = atoi( subToken->ascii() );
            ++subToken;
            unsigned nodeIndex = atoi( subToken->ascii() );

            NLR::NetworkLevelReasoner *nlr = inputQuery.getNetworkLevelReasoner();
            if ( !nlr )
                throw InputParserError( InputParserError::NETWORK_LEVEL_REASONING_DISABLED );

            if ( nlr->getNumberOfLayers() < layerIndex )
                throw InputParserError( InputParserError::HIDDEN_VARIABLE_DOESNT_EXIST_IN_NLR );

            const NLR::Layer *layer = nlr->getLayer( layerIndex );
            if ( layer->getSize() < nodeIndex || !layer->neuronHasVariable( nodeIndex ) )
                throw InputParserError( InputParserError::HIDDEN_VARIABLE_DOESNT_EXIST_IN_NLR );

            variable = layer->neuronToVariable( nodeIndex );
        }

        if ( type == Equation::GE )
        {
            if ( inputQuery.getLowerBound( variable ) < scalar )
                inputQuery.setLowerBound( variable, scalar );
        }
        else if ( type == Equation::LE )
        {
            if ( inputQuery.getUpperBound( variable ) > scalar )
                inputQuery.setUpperBound( variable, scalar );
        }
        else
        {
            ASSERT( type == Equation::EQ );

            if ( inputQuery.getLowerBound( variable ) < scalar )
                inputQuery.setLowerBound( variable, scalar );
            if ( inputQuery.getUpperBound( variable ) > scalar )
                inputQuery.setUpperBound( variable, scalar );
        }
    }
    else
    {
        // Normal case: add as an equation
        Equation equation( type );
        equation.setScalar( scalar );

        while ( it != tokens.rend() )
        {
            String token = (*it).trim();

            bool inputVariable = token.contains( "x" );
            bool outputVariable = token.contains( "y" );

            if ( !( inputVariable ^ outputVariable ) )
                throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

            List<String> subTokens;
            if ( inputVariable )
                subTokens = token.tokenize( "x" );
            else
                subTokens = token.tokenize( "y" );

            if ( subTokens.size() != 2 )
                throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

            unsigned justIndex = atoi( subTokens.rbegin()->ascii() );
            unsigned variable;

            if ( inputVariable )
            {
                ASSERT( justIndex < inputQuery.getNumInputVariables() );
                variable = inputQuery.inputVariableByIndex( justIndex );
            }
            else
            {
                ASSERT( justIndex < inputQuery.getNumOutputVariables() );
                variable = inputQuery.outputVariableByIndex( justIndex );
            }

            String coefficientString = *subTokens.begin();
            double coefficient;
            if ( coefficientString == "+" )
                coefficient = 1;
            else if ( coefficientString == "-" )
                coefficient = -1;
            else
                coefficient = atof( coefficientString.ascii() );

            equation.addAddend( coefficient, variable );
            ++it;
        }

        inputQuery.addEquation( equation );
    }
}

Equation::EquationType PropertyParser::extractRelationSymbol( const String &token )
{
    if ( token == ">=" )
        return Equation::GE;
    if ( token == "<=" )
        return Equation::LE;
    if ( token == "=" )
        return Equation::EQ;

    throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
