/*********************                                                        */
/*! \file BerkeleyNeuralNetwork.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "BerkeleyNeuralNetwork.h"
#include "CommonError.h"

BerkeleyNeuralNetwork::Equation::Equation()
    : _constant( 0 )
{
}

void BerkeleyNeuralNetwork::Equation::dump()
{
    printf( "lhs: %u\n", _lhs );
    printf( "rhs:\n" );
    for ( const auto &it : _rhs )
        printf( "\t%lf * %u\n", it._coefficient, it._var );

    printf( "\t%lf\n", _constant );
}

BerkeleyNeuralNetwork::BerkeleyNeuralNetwork( const String &path )
    : _file( path )
    , _maxVar( 0 )
{
    _file.open( File::MODE_READ );
}

unsigned BerkeleyNeuralNetwork::getNumVariables() const
{
    return _allVars.size();
}

unsigned BerkeleyNeuralNetwork::getNumEquations() const
{
    return _equations.size();
}

unsigned BerkeleyNeuralNetwork::getNumInputVariables() const
{
    return _inputVars.size();
}

unsigned BerkeleyNeuralNetwork::getNumOutputVariables() const
{
    return _outputVars.size();
}

unsigned BerkeleyNeuralNetwork::getNumReluNodes() const
{
    return _bToF.size();
}

Set<unsigned> BerkeleyNeuralNetwork::getInputVariables() const
{
    return _inputVars;
}

Set<unsigned> BerkeleyNeuralNetwork::getOutputVariables() const
{
    return _outputVars;
}

Map<unsigned, unsigned> BerkeleyNeuralNetwork::getFToB() const
{
    return _fToB;
}

List<BerkeleyNeuralNetwork::Equation> BerkeleyNeuralNetwork::getEquations() const
{
    return _equations;
}

void BerkeleyNeuralNetwork::parseFile()
{
    String line;

    try
    {
        while ( true )
        {
            line = _file.readLine();
            processLine( line );
        }
    }
    catch ( const CommonError &e )
    {
        if ( e.getCode() != CommonError::READ_FAILED )
            throw e;
    }

    printf( "Max var: %u. Number of vars: %u. Number of LHS vars: %u. Number of equations: %u\n",
            _maxVar, _allVars.size(), _allLhsVars.size(), _equations.size() );

    _inputVars = Set<unsigned>::difference( _allVars, _allLhsVars );
    _outputVars = Set<unsigned>::difference( _allVars, _allRhsVars );

    printf( "Input vars: count = %u\n", _inputVars.size() );

    printf( "Output vars: count = %u\n", _outputVars.size() );
}

void BerkeleyNeuralNetwork::processLine( const String &line )
{
    if ( line.contains( "Relu" ) )
        processReluLine( line );
    else
        processEquationLine( line );
}

void BerkeleyNeuralNetwork::processReluLine( const String &line )
{
    List<String> tokens = line.tokenize( "=" );
    if ( tokens.size() != 2 )
    {
        printf( "Error! Expected 2 tokens\n" );
        exit( 1 );
    }

    List<String>::iterator it = tokens.begin();
    String firstToken = *it;
    ++it;
    String secondToken = *it;

    unsigned f = varStringToUnsigned( firstToken.trim() );
    String reluB = secondToken.trim();
    if ( !reluB.contains( "Relu" ) )
    {
        printf( "Error! Not a valid reluB string: %s\n", reluB.ascii() );
        exit( 1 );
    }

    unsigned b = varStringToUnsigned( reluB.substring( 5, reluB.length() - 6 ) );

    _bToF[b] = f;
    _fToB[f] = b;
    processVar( b );

    _allRhsVars.insert( b );
    _allLhsVars.insert( f );
}

unsigned BerkeleyNeuralNetwork::varStringToUnsigned( String varString )
{
    unsigned result = atoi( varString.substring( 1, varString.length() ).ascii() );
    processVar( result );
    return result;
}

void BerkeleyNeuralNetwork::processVar( unsigned var )
{
    _allVars.insert( var );
    if ( var > _maxVar )
        _maxVar = var;
}

void BerkeleyNeuralNetwork::processEquationLine( const String &line )
{
    BerkeleyNeuralNetwork::Equation equation;
    equation._index = _equations.size();

    List<String> tokens = line.tokenize( "=" );
    if ( tokens.size() != 2 )
    {
        printf( "Error! Expected 2 tokens\n" );
        exit( 1 );
    }

    List<String>::iterator it = tokens.begin();
    String firstToken = *it;
    ++it;
    String secondToken = *it;

    equation._lhs = varStringToUnsigned( firstToken.trim() );
    _allLhsVars.insert( equation._lhs );

    List<String> rhsTokens = ( secondToken.trim() ).tokenize( "+" );

    for ( it = rhsTokens.begin(); it != rhsTokens.end(); ++it )
    {
        String token = it->trim();

        if ( token.contains( "*" ) )
        {
            // Coefficient times variable
            List<String> varAndCoefficient = token.tokenize( "*" );
            BerkeleyNeuralNetwork::Equation::RhsPair rhsPair;

            List<String>::iterator it2 = varAndCoefficient.begin();

            rhsPair._coefficient = atof( it2->trim().ascii() );
            ++it2;
            rhsPair._var = varStringToUnsigned( it2->trim() );

            equation._rhs.append( rhsPair );
            _allRhsVars.insert( rhsPair._var );
        }
        else
        {
            // Single element. Either a varibale without a coefficient, or a constatnt
            if ( token.contains( "v" ) )
            {
                BerkeleyNeuralNetwork::Equation::RhsPair rhsPair;

                rhsPair._coefficient = 1.0;
                rhsPair._var = varStringToUnsigned( token );

                equation._rhs.append( rhsPair );
                _allRhsVars.insert( rhsPair._var );
            }
            else
                equation._constant = atof( token.ascii() );
        }
    }

    _equations.append( equation );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
