/*********************                                                        */
/*! \file PropertyParser.cpp
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
    double scalar = extractScalar( *it );
    ++it;
    Equation::EquationType type = extractSign( *it );
    ++it;

    Equation equation( type );
    equation.setScalar( scalar );

    // Now extract the addends
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

Equation::EquationType PropertyParser::extractSign( const String &token )
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
