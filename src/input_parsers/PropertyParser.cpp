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
#include "PropertyParser.h"

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
    // TODO: ASSERT(isScalar(*it))
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

        if ( !( inputVariable xor outputVariable ) )
            throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

        List<String> subTokens;
        if ( inputVariable )
            subTokens = token.tokenize( "x" );
        else
            subTokens = token.tokenize( "y" );

        if ( subTokens.size() != 1 )
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
        // TODO: ASSERT (symbol is equation)
        Equation equation( type );
        equation.setScalar( scalar );

        while ( it != tokens.rend() )
        {
            String token = (*it).trim();

            bool inputVariable = token.contains( "x" );
            bool outputVariable = token.contains( "y" );

            if ( !( inputVariable xor outputVariable ) )
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

double PropertyParser::extractScalar( const String &token )
{
    return atof( token.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
