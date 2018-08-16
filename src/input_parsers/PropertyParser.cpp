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

    if ( tokens.size() != 3 )
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, line.ascii() );

    auto it = tokens.begin();
    unsigned variable = extractVariable( *it, inputQuery );
    ++it;
    Sign sign = extractSign( *it );
    ++it;
    double scalar = extractScalar( *it );

    if ( sign == GEQ )
    {
        if ( inputQuery.getLowerBound( variable ) < scalar )
            inputQuery.setLowerBound( variable, scalar );
    }
    else
    {
        if ( inputQuery.getUpperBound( variable ) > scalar )
            inputQuery.setUpperBound( variable, scalar );
    }
}

unsigned PropertyParser::extractVariable( const String &token, const InputQuery &inputQuery )
{
    bool inputVariable = token.contains( "x" );
    bool outputVariable = token.contains( "y" );

    if ( !( inputVariable xor outputVariable ) )
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, token.ascii() );

    String justIndex = token.substring( 1, token.length() - 1 );

    if ( inputVariable )
    {
        ASSERT( atoi( justIndex.ascii() ) < (int)inputQuery.getNumInputVariables() );
        return inputQuery.inputVariableByIndex( atoi( justIndex.ascii() ) );
    }
    else
    {
        ASSERT( atoi( justIndex.ascii() ) < (int)inputQuery.getNumOutputVariables() );
        return inputQuery.outputVariableByIndex( atoi( justIndex.ascii() ) );
    }
}

PropertyParser::Sign PropertyParser::extractSign( const String &token )
{
    if ( token == ">=" )
        return GEQ;
    if ( token == "<=" )
        return LEQ;

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
