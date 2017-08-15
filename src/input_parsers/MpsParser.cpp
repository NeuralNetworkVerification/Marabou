/*********************                                                        */
/*! \file MpsParser.cpp
** \verbatim
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "MpsParser.h"
#include "MString.h"

MpsParser::MpsParser( const char *filename )
{
    // Parses a MPS format file
    parse( filename );
}
 
unsigned MpsParser::getNumVars() const
{
    return _numVars;
}

unsigned MpsParser::getNumVarsInclAux() const
{
    return _numVars + _numEqns;
}

unsigned MpsParser::getNumEqns() const
{
    return _numEqns;
}

void MpsParser::parse( const char* filename )
{
    // Load file and check if it exists
    FILE *fstream = fopen( filename, "r" );

    if ( fstream == NULL )
	{
	    // TODO: throw error
	    return;
	}

    int bufferSize = 10240;
    char *buffer = new char[bufferSize];
    char *line;
    char *token;

    // Read MPS file
    line = fgets( buffer, bufferSize, fstream ); // Skip header line: NAME	TESTPROB
    line = fgets( buffer, bufferSize, fstream ); // Skip second line: ROWS

    unsigned numRows = 0, numCols = 0;
    while( true )
	{
	    line = fgets( buffer, bufferSize, fstream );
	    token = strtok( line, DELIM );
	    //		printf( "%s", token );

	    if ( strcmp(token, "COLUMNS") == 0)
		break; // next part

	    numRows++;
	    parseRow( token );
	}

    printf("Parsed %d rows.\n", numRows);
    while( true )
	{
	    line = fgets( buffer, bufferSize, fstream );
	    token = strtok( line, DELIM );
	    //		printf("%s\n", token);

	    if ( strcmp(token, "RHS") == 0)
		break; // next part

	    numCols++;
	    parseColumn( token );
	}
    printf("Parsed %d lines of columns.\n", numCols);
    while( true )
	{
	    line = fgets( buffer, bufferSize, fstream );
	    token = strtok( line, DELIM );

	    if ( strcmp( token, "BOUNDS" ) == 0 || strcmp( token, "RANGES" ) == 0 || strcmp( token, "ENDATA" ) == 0 )
		break;

	    parseRhs( token );
	}

    if ( strcmp( token, "RANGES" ) == 0 )
    {
	while( true )
	{
	    line = fgets( buffer, bufferSize, fstream );
	    token = strtok( line, DELIM );

	    if ( strcmp( token, "BOUNDS") == 0 || strcmp( token, "ENDATA" ) == 0 )
		break;
	    parseRanges( token );
	}
    }

    if ( strcmp( token, "BOUNDS" ) == 0 )
    {
	while( true )
	{
	    line = fgets( buffer, bufferSize, fstream );
	    token = strtok( line, DELIM );

	    if ( strcmp( token, "ENDATA") == 0 || strcmp( token, "SOS") == 0 )
		break; // end
	    parseBounds( token );
	}
    }

    parseRemainingBounds();
    
    delete[] buffer;
}

void MpsParser::parseRemainingBounds()
{
    // Variables with no bounds specified have LB of 0 and UB of inf.
    for ( unsigned i = 0; i < _numVars; ++i )
    {
	if ( !_varToLowerBounds.exists( i ) &&
	     (!_varToUpperBounds.exists( i ) || _varToUpperBounds[i] >= 0 ) )
	    _varToLowerBounds[i] = 0;
    }
}
void MpsParser::parseRanges( const char *token )
{
    // TODO
    printf("%s", token );
    printf(" Not configured to handle 'RANGES' in MPS file format.\nStatus: PARSE_FAILURE\n" );
    exit(1);
}

void MpsParser::parseBounds( const char *token )
{
    if ( strcmp( token, "BV" ) == 0 || strcmp( token, "UI") == 0 || strcmp( token, "LI" ) == 0 || strcmp( token, "SC" ) == 0) {
	printf( "Only configured to handle UP, LO, FX, FR, MI, and PL bounds.\nStatus: PARSE_FAILURE\n" );
	exit(1);
    }
    strtok( NULL, DELIM ); // bound name, ignore
    String var = String( strtok( NULL, DELIM ) ); // column name
    
    unsigned varIndex = _varToIndex[var];
    double value = atof( strtok( NULL, DELIM ) ); // bound value
    
    // Log bounds for each different bound type
    if ( strcmp( token, "UP" ) == 0) {
	if ( !_varToUpperBounds.exists(varIndex) || FloatUtils::gt(_varToUpperBounds[varIndex], value ) ) {
	    _varToUpperBounds[varIndex] = value;
	}
    } else if ( strcmp( token, "LO" ) == 0 ) {
	if ( !_varToLowerBounds.exists(varIndex) || FloatUtils::lt(_varToLowerBounds[varIndex], value ) ) {
	    _varToLowerBounds[varIndex] = value;
	}
    } else if ( strcmp( token, "FX" ) == 0 ) {
	// fixed = both upper and lower
	if ( !_varToUpperBounds.exists(varIndex) || FloatUtils::gt(_varToUpperBounds[varIndex], value ) ) {
	    _varToUpperBounds[varIndex] = value;
	}
	if ( !_varToLowerBounds.exists(varIndex) || FloatUtils::lt(_varToLowerBounds[varIndex], value ) ) {
	    _varToLowerBounds[varIndex] = value;
	}
    } else if ( strcmp( token, "FR" ) == 0 ) {
	if ( _varToUpperBounds.exists(varIndex) || _varToLowerBounds.exists( varIndex ) )
	    throw InputParserError( InputParserError::UNEXPECTED_INPUT, "Conflicting bounds.");
    } else if ( strcmp( token, "MI" ) == 0 ) {
	if ( _varToLowerBounds.exists( varIndex ) )
	    throw InputParserError( InputParserError::UNEXPECTED_INPUT, "Conflicting bounds.");
	_varToUpperBounds[varIndex] = 0.0;
    } else if ( strcmp( token, "PL" ) == 0 ) {
	if ( _varToUpperBounds.exists( varIndex ) )
	    throw InputParserError( InputParserError::UNEXPECTED_INPUT, "Conflicting bounds.");
	_varToLowerBounds[varIndex] = 0.0;
    } else {
	printf( "Only configured to handle UP, LO, FX, FR, MI, and PL bounds.\nStatus: PARSE_FAILURE\n" );
	exit(1);
    }
}

void MpsParser::parseRhs( const char *token )
{
    token = strtok( NULL, DELIM ); // row name
    while ( token != NULL ) // read an (eqn, rhs) pair
    {
	unsigned eqnIndex = _eqnToIndex[String( token )];
	token = strtok( NULL, DELIM); // value
	
	_eqnToRhs[eqnIndex] = atof( token );
	token = strtok( NULL, DELIM); // next eqn name
    }
}

String MpsParser::getVarName( unsigned index ) const
{
    return _indexToVar[index];
}

String MpsParser::getEqnName( unsigned index ) const
{
    return _indexToEqn[index];
}

String MpsParser::ftos( double val ) const
{
    // converts float to string
    char buf[50];
    sprintf( buf, "%f", val );
    return String( buf );
}
String MpsParser::getUpperBound( unsigned index ) const
{
    return _varToUpperBounds.exists( index ) ? ftos( _varToUpperBounds[index] ) : String("NA");
}

String MpsParser::getLowerBound( unsigned index ) const
{
    return _varToLowerBounds.exists( index ) ? ftos( _varToLowerBounds[index] ) : String("NA");
}

String MpsParser::getEqnValue( unsigned index, const InputQuery &inputQuery ) const
{
    double value = 0.0;
    const Map<unsigned, double> &coeffs = _eqnToCoeffs[index];
    Map<unsigned, double>::const_iterator it;

    for ( it = coeffs.begin(); it != coeffs.end(); ++it )
    {
	value += it->second * inputQuery.getSolutionValue( it->first);
    }

    return ftos( value );
}

String MpsParser::getEqnLB( unsigned index ) const
{
    return _eqnToRowType[index] == RowType::G || _eqnToRowType[index] == RowType::E ?
	ftos( _eqnToRhs[index] ) : String( "NA" );
}

String MpsParser::getEqnUB( unsigned index ) const
{
    return _eqnToRowType[index] == RowType::L || _eqnToRowType[index] == RowType::E ?
	ftos( _eqnToRhs[index] ) : String( "NA" );
}

void MpsParser::parseColumn( const char *token )
{
    String name = String( token );
    unsigned varIndex;

    if ( !_varToIndex.exists( name ) )
    {
	    varIndex = _numVars;
	    _varToIndex[name] = varIndex;
	    _indexToVar[varIndex] = name;
	    _numVars++;
    }
    else
    {
	    varIndex = _varToIndex[name];
    }

    token = strtok( NULL, DELIM );
    while ( token != NULL ) // read an (eqn, coeff) pair
    {
	//	    printf("tok: %s", token);
	bool validEqn = _eqnToIndex.exists( String( token ) );

	if ( validEqn) {
	    unsigned eqnIndex = _eqnToIndex[String( token )];
	    token = strtok( NULL, DELIM ); // coeff

	    _eqnToCoeffs[eqnIndex][varIndex] = atof( token );
	    token = strtok( NULL, DELIM); // next eqn name
	} else {
	    token = strtok( NULL, DELIM ); // coeff
	    token = strtok( NULL, DELIM ); // next eqn name
	}
    }
}

void MpsParser::parseRow( const char *token )
{
    unsigned eqnIndex = _numEqns;
    
    char *nameC = strtok( NULL, "\t\n ");
    //    printf("%s: %s\n", token, nameC);

    // store row type
    switch( token [0] ){
    case 'E':
	_eqnToRowType[eqnIndex] = RowType::E;
	break;
    case 'L':
	_eqnToRowType[eqnIndex] = RowType::L;
	break;
    case 'G':
	_eqnToRowType[eqnIndex] = RowType::G;
	break;
    default:
	// ignore
	return;
    }

    // save equation number
    _numEqns++;
    _eqnToIndex[String( nameC )] = eqnIndex;
    _indexToEqn[eqnIndex] = String( nameC );
}

void MpsParser::generateQuery( InputQuery &inputQuery )
{
    inputQuery.setNumberOfVariables( _numVars + _numEqns );
    // TODO: incl numEqns for aux variables, but obj function doesn't have aux var

    setBounds( inputQuery );
    setEqns( inputQuery );
}

void MpsParser::setBounds( InputQuery &inputQuery )
{
    Map<unsigned, double>::iterator it;
    for ( it = _varToUpperBounds.begin(); it != _varToUpperBounds.end(); ++it )
    {
	inputQuery.setUpperBound( it->first, it->second );
    }
    for ( it = _varToLowerBounds.begin(); it != _varToLowerBounds.end(); ++it )
	{
	inputQuery.setLowerBound( it->first, it->second );
    }
}

void MpsParser::setEqns( InputQuery &inputQuery )
{
    for ( unsigned eqnIndex = 0; eqnIndex < _numEqns; ++eqnIndex )
    {    // for each coeff, set in input query

	if ( _eqnToRowType[eqnIndex] == RowType::N )
	{
	    printf("Ignoring objective. TODO: transform optval into another constraint.\n");
	    continue;
	}
	
	Equation eqn;

	unsigned auxVarIndex = eqnIndex + _numVars; // TODO: obj function doesn't have aux var
	setEqn( eqn, eqnIndex, auxVarIndex ); // sets coeffs and marks aux vars

	// Set bounds on aux vars
	switch ( _eqnToRowType[eqnIndex] ) {
	case RowType::E:
	    inputQuery.setLowerBound( auxVarIndex, 0 );
	    inputQuery.setUpperBound( auxVarIndex, 0 );
	    break;
	case RowType::L:
	    inputQuery.setLowerBound( auxVarIndex, 0 );
	    break;
	case RowType::G:
	    inputQuery.setUpperBound( auxVarIndex, 0 );
	    break;
	default:
	    // Do nothing
	    break;
	    // TODO: transform the optval into another constraint
	}
	
	// Add eqn to inputQuery
	inputQuery.addEquation( eqn );

    }
}

void MpsParser::setEqn( Equation &eqn, unsigned eqnIndex, unsigned auxVarIndex )
{
    Map<unsigned, double> &coeffs = _eqnToCoeffs[eqnIndex];
    Map<unsigned, double>::iterator it;

    for ( it = coeffs.begin(); it != coeffs.end(); ++ it)
    {
		eqn.addAddend( it->second, it->first ); // val, index
    }

    // Aux variable
    eqn.addAddend( 1, auxVarIndex );
    eqn.markAuxiliaryVariable( auxVarIndex );

    // Rhs
    eqn.setScalar( _eqnToRhs[eqnIndex] );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
