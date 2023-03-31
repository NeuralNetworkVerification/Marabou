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
#include "Pair.h"


enum VariableType {
    Input = 0,
    Output = 1,
    Hidden = 2
};

static Equation::EquationType parseEquationType( const String &token )
{
    if ( token == ">=" )
        return Equation::GE;
    if ( token == "<=" )
        return Equation::LE;
    if ( token == "=" )
        return Equation::EQ;

    throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );
}

static double parseScalar( const String &token )
{
    std::string::size_type end;
    double value = std::stod( token.ascii(), &end );
    if ( end != token.length() )
    {
        String errorMessage = Stringf( "%s not a scalar", token.ascii() );
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, errorMessage.ascii() );
    }
    return value;
}

static double parseCoefficient ( const String &token )
{
    if ( token == "+" )
        return 1;
    else if ( token == "-" )
        return -1;
    else
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
            bool isEmpty = line.length() == 0;
            bool isComment = line.substring(0,2) == "//";
            if ( !isEmpty && !isComment )
            {
                parseEquation( line, inputQuery );
            }
        }
    }
    catch ( const CommonError &e )
    {
        // A "READ_FAILED" is how we know we're out of lines
        if ( e.getCode() != CommonError::READ_FAILED )
            throw e;
    }
}

void PropertyParser::parseEquation( const String &line, InputQuery &inputQuery )
{
    List<String> tokens = line.tokenize( " " );

    // Malformed, each line must have at least one variable, a relation and a coefficient.
    if ( tokens.size() < 3 ) {
        String errorMessage = Stringf( "Invalid equation '%s'", line.ascii() );
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, errorMessage.ascii() );
    }

    auto it = tokens.rbegin();
    double scalar = parseScalar( *it );
    ++it;
    Equation::EquationType equationType = parseEquationType( *it );
    ++it;

    // Now extract the addends. In the special case where we only have
    // one addend, we add this equation as a bound. Otherwise, we add
    // as an equation.
    if ( tokens.size() == 3 )
    {
        // Special case: add as a bound on the variable
        String token = (*it).trim();
        Pair<double, unsigned> addend = parseAddend ( token, inputQuery );

        double coefficient = addend.first();
        // Currently don't support coefficients on bounds
        // (see https://github.com/NeuralNetworkVerification/Marabou/issues/625)
        if ( coefficient != 1 )
        {
            String errorMessage = Stringf( "'%s' - coefficients for bounds not supported", line.ascii() );
            throw InputParserError( InputParserError::UNEXPECTED_INPUT, errorMessage.ascii() );
        }

        unsigned variable = addend.second();

        if ( equationType == Equation::GE )
        {
            if ( inputQuery.getLowerBound( variable ) < scalar )
                inputQuery.setLowerBound( variable, scalar );
        }
        else if ( equationType == Equation::LE )
        {
            if ( inputQuery.getUpperBound( variable ) > scalar )
                inputQuery.setUpperBound( variable, scalar );
        }
        else
        {
            ASSERT( equationType == Equation::EQ );

            if ( inputQuery.getLowerBound( variable ) < scalar )
                inputQuery.setLowerBound( variable, scalar );
            if ( inputQuery.getUpperBound( variable ) > scalar )
                inputQuery.setUpperBound( variable, scalar );
        }
    }
    else
    {
        // Normal case: add as a new equation over multiple variables
        Equation equation( equationType );
        equation.setScalar( scalar );

        while ( it != tokens.rend() )
        {
            String token = (*it).trim();
            Pair<double, unsigned> addend = parseAddend ( token, inputQuery );
            double coefficient = addend.first();
            unsigned variable = addend.second();
            equation.addAddend( coefficient, variable );
            ++it;
        }

        inputQuery.addEquation( equation );
    }
}

// Returns (coefficient, variableNumber)
Pair<double, unsigned> PropertyParser::parseAddend ( const String &token , InputQuery &inputQuery )
{
    String symbol;
    VariableType variableType;
    unsigned variableTypeCount = 0;

    if ( token.contains ("x") )
    {
        variableType = VariableType::Input;
        symbol = "x";
        ++variableTypeCount;
    }

    if ( token.contains ("y") )
    {
        variableType = VariableType::Output;
        symbol = "y";
        ++variableTypeCount;
    }

    if ( token.contains("h") )
    {
        variableType = VariableType::Hidden;
        symbol = "h";
        ++variableTypeCount;
    }

    // Make sure that we have identified precisely one kind of variable
    if ( variableTypeCount != 1 )
    {
        String errorMessage = Stringf( "'%s' does not contain a valid variable name", token.ascii() );
        throw InputParserError( InputParserError::INVALID_VARIABLE_NAME, errorMessage.ascii() );
    }

    // Extract variable and coefficient strings
    List<String> subTokens = token.tokenize( symbol );
    String coefficientString;
    String variableIDString;
    if ( subTokens.size() == 1)
    {
        coefficientString = "+";
        variableIDString = *subTokens.begin();
    }
    else if ( subTokens.size() == 2 )
    {
        coefficientString = *subTokens.begin();
        variableIDString = *subTokens.rbegin();
    }
    else
    {
        String errorMessage = Stringf( "'%s' is not a valid addend", token.ascii() );
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, errorMessage.ascii() );
    }

    // Parse coefficient
    double coefficient = parseCoefficient ( coefficientString );

    // Parse variable
    unsigned variable;
    if ( variableType == VariableType::Input )
    {
        unsigned justIndex = atoi( variableIDString.ascii() );
        ASSERT( justIndex < inputQuery.getNumOutputVariables() );
        variable = inputQuery.inputVariableByIndex( justIndex );
    }
    else if ( variableType == VariableType::Output )
    {
        unsigned justIndex = atoi( variableIDString.ascii() );
        ASSERT( justIndex < inputQuery.getNumOutputVariables() );
        variable = inputQuery.outputVariableByIndex( justIndex );
    }
    else
    {
        // These variables are of the form h_2_5
        subTokens = variableIDString.tokenize( "_" );

        if ( subTokens.size() != 2 )
        {
            String errorMessage = Stringf( "'%s' does not contain valid hidden variable indices", token.ascii() );
            throw InputParserError( InputParserError::INVALID_HIDDEN_VARIABLE_INDICES, errorMessage.ascii() );
        }

        auto subToken = subTokens.begin();
        unsigned layerIndex = atoi( subToken->ascii() );
        ++subToken;
        unsigned nodeIndex = atoi( subToken->ascii() );

        NLR::NetworkLevelReasoner *nlr = inputQuery.getNetworkLevelReasoner();
        if ( !nlr )
        {
            String errorMessage = Stringf( "Cannot refer to hidden variable '%s' when network level reasoning is disabled", token.ascii() );
            throw InputParserError( InputParserError::NETWORK_LEVEL_REASONING_DISABLED, errorMessage.ascii() );
        }

        if ( nlr->getNumberOfLayers() < layerIndex )
        {
            String errorMessage = Stringf( "Layer index '%d' for hidden variable '%s' is invalid", layerIndex, token.ascii() );
            throw InputParserError( InputParserError::HIDDEN_VARIABLE_DOESNT_EXIST_IN_NLR );
        }

        const NLR::Layer *layer = nlr->getLayer( layerIndex );
        if ( layer->getSize() < nodeIndex || !layer->neuronHasVariable( nodeIndex ) )
        {
            String errorMessage = Stringf( "Node index '%d' for hidden variable '%s' is invalid", nodeIndex, token.ascii() );
            throw InputParserError( InputParserError::HIDDEN_VARIABLE_DOESNT_EXIST_IN_NLR );
        }

        variable = layer->neuronToVariable( nodeIndex );
    }

    return Pair<double,unsigned>(coefficient, variable);
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
