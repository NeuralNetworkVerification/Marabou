/*********************                                                        */
/*! \file VnnLibParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A parser for properties encoded in VNN-LIB format.

**/

#include "VnnLibParser.h"

#include "DisjunctionConstraint.h"
#include "Equation.h"
#include "File.h"
#include "InputParserError.h"

#include <boost/regex.hpp>

static double extractScalar( const String &token )
{
    std::string::size_type end;
    double value = std::stod( token.ascii(), &end );
    if ( end != token.length() )
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                Stringf( "%s not a scalar", token.ascii() ).ascii() );
    }
    return value;
}

static String readVnnlibFile( const String &vnnlibFilePath )
{
    if ( !File::exists( vnnlibFilePath ) )
    {
        std::cout << "Error: the specified property file " << vnnlibFilePath.ascii()
                  << " doesn't exist!" << std::endl;
        throw InputParserError( InputParserError::FILE_DOESNT_EXIST, vnnlibFilePath.ascii() );
    }

    File vnnlibFile( vnnlibFilePath );
    vnnlibFile.open( File::MODE_READ );

    String vnnlibContent;

    try
    {
        while ( true )
        {
            String line = vnnlibFile.readLine().trim();
            if ( line == "" || line.substring( 0, 1 ) == ";" )
            {
                continue;
            }
            vnnlibContent += line;
        }
    }
    catch ( const CommonError &e )
    {
        // A "READ_FAILED" is how we know we're out of lines
        if ( e.getCode() != CommonError::READ_FAILED )
            throw e;
    }

    return vnnlibContent;
}

void VnnLibParser::parse( const String &vnnlibFilePath, IQuery &inputQuery )
{
    String vnnlibContent = readVnnlibFile( vnnlibFilePath );

    parseVnnlib( vnnlibContent, inputQuery );
}

void VnnLibParser::parseVnnlib( const String &vnnlibContent, IQuery &inputQuery )
{
    boost::regex re( R"(\(|\)|[\w\-\\.]+|<=|>=|\+|-|\*)" );

    // Use a Boost cregex_token_iterator to tokenize a C-string range
    auto tokens_begin = boost::cregex_token_iterator(
        vnnlibContent.ascii(), vnnlibContent.ascii() + vnnlibContent.length(), re );
    auto tokens_end = boost::cregex_token_iterator();

    Vector<String> all_tokens;

    // Iterate over all matches
    for ( boost::cregex_token_iterator it = tokens_begin; it != tokens_end; ++it )
    {
        // Each match is a boost::csub_match, from which we can get a std::string
        boost::csub_match match = *it;

        // Convert match.str() to your custom String type
        // Assuming your String can be constructed from a const char*
        String match_str( match.str().c_str() );

        all_tokens.append( match_str );
    }

    parseScript( all_tokens, inputQuery );
}

int VnnLibParser::parseScript( const Vector<String> &tokens, IQuery &inputQuery )
{
    int index = 0;

    while ( (unsigned)index < tokens.size() )
    {
        ASSERT( tokens[index] == "(" )
        index = parseCommand( index + 1, tokens, inputQuery );
        ASSERT( tokens[index] == ")" )

        ++index;
    }

    return index;
}

int VnnLibParser::parseCommand( int index, const Vector<String> &tokens, IQuery &inputQuery )
{
    const String &command_name = tokens[index];

    if ( command_name == "declare-const" )
    {
        index = parseDeclareConst( index + 1, tokens, inputQuery );
    }
    else if ( command_name == "assert" )
    {
        index = parseAssert( index + 1, tokens, inputQuery );
    }
    else
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, command_name.ascii() );
    }

    return index;
}

int VnnLibParser::parseDeclareConst( int index, const Vector<String> &tokens, IQuery &inputQuery )
{
    const String &varName = tokens[index];
    const String &varType = tokens[++index];
    ++index;

    if ( varType != "Real" )
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                "Does not support variable types other than 'Real'" );
    }

    List<String> varTokens = varName.tokenize( "_" );
    if ( varTokens.size() != 2 )
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, varName.ascii() );
    }

    const String varKind = varTokens.front();
    const String varIdxStr = varTokens.back();

    for ( unsigned int i = 0; i < varIdxStr.length(); ++i )
    {
        if ( !std::isdigit( varIdxStr[i] ) )
        {
            throw InputParserError( InputParserError::UNEXPECTED_INPUT, varName.ascii() );
        }
    }

    unsigned int varIdx = atoi( varIdxStr.ascii() );

    if ( varKind == "X" )
    {
        if ( varIdx >= inputQuery.getNumInputVariables() )
        {
            throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE,
                                    varName.ascii() );
        }

        _varMap.insert( varName, inputQuery.inputVariableByIndex( varIdx ) );
    }
    else if ( varKind == "Y" )
    {
        if ( varIdx >= inputQuery.getNumOutputVariables() )
        {
            throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE,
                                    varName.ascii() );
        }

        _varMap.insert( varName, inputQuery.outputVariableByIndex( varIdx ) );
    }
    else
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, varName.ascii() );
    }

    return index;
}

int VnnLibParser::parseAssert( int index, const Vector<String> &tokens, IQuery &inputQuery )
{
    ASSERT( tokens[index] == "(" );
    ++index;

    const String &op = tokens[index];
    if ( op == "<=" || op == ">=" || op == "and" )
    {
        List<Equation> equations;
        index = parseCondition( index, tokens, equations );
        for ( const auto &eq : equations )
        {
            if ( eq._addends.size() == 1 )
            {
                const Equation::Addend &addend = eq._addends.front();
                if ( addend._coefficient < 0 )
                {
                    inputQuery.setLowerBound( addend._variable, eq._scalar / addend._coefficient );
                }
                else if ( addend._coefficient > 0 )
                {
                    inputQuery.setUpperBound( addend._variable, eq._scalar / addend._coefficient );
                }
                else if ( eq._scalar < 0 )
                {
                    throw InputParserError(
                        InputParserError::UNEXPECTED_INPUT,
                        Stringf( "Illegal vnnlib constraint: 0 < %f", eq._scalar ).ascii() );
                }
                else
                {
                    continue;
                }
            }
            else
            {
                inputQuery.addEquation( eq );
            }
        }
    }
    else if ( op == "or" )
    {
        List<PiecewiseLinearCaseSplit> disjunctList;
        ++index;
        while ( tokens[index] != ")" )
        {
            List<Equation> equations;
            index = parseCondition( index + 1, tokens, equations );

            PiecewiseLinearCaseSplit split;
            for ( const auto &eq : equations )
            {
                if ( eq._addends.size() == 1 )
                {
                    // Add bounds as tightenings
                    unsigned var = eq._addends.front()._variable;
                    double coeff = eq._addends.front()._coefficient;
                    if ( coeff == 0 )
                        throw CommonError( CommonError::DIVISION_BY_ZERO,
                                           "Zero coefficient encountered in vnnlib constraint" );
                    double scalar = eq._scalar / coeff;
                    Equation::EquationType type = eq._type;

                    if ( type == Equation::EQ )
                    {
                        split.storeBoundTightening( Tightening( var, scalar, Tightening::LB ) );
                        split.storeBoundTightening( Tightening( var, scalar, Tightening::UB ) );
                    }
                    else if ( ( type == Equation::GE && coeff > 0 ) ||
                              ( type == Equation::LE && coeff < 0 ) )
                        split.storeBoundTightening( Tightening( var, scalar, Tightening::LB ) );
                    else
                        split.storeBoundTightening( Tightening( var, scalar, Tightening::UB ) );
                }
                else
                {
                    split.addEquation( eq );
                }
            }
            disjunctList.append( split );
        }

        inputQuery.addPiecewiseLinearConstraint( new DisjunctionConstraint( disjunctList ) );
        ++index;
    }
    else
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT, op.ascii() );
    }

    return index;
}

int VnnLibParser::parseCondition( int index,
                                  const Vector<String> &tokens,
                                  List<Equation> &equations )
{
    const String &op = tokens[index];

    if ( op == "<=" )
    {
        Term arg1, arg2;
        index = parseTerm( index + 1, tokens, arg1 );
        index = parseTerm( index + 1, tokens, arg2 );

        equations.append( processLeConstraint( arg1, arg2 ) );
    }
    else if ( op == ">=" )
    {
        Term arg1, arg2;
        index = parseTerm( index + 1, tokens, arg1 );
        index = parseTerm( index + 1, tokens, arg2 );

        equations.append( processLeConstraint( arg2, arg1 ) );
    }
    else if ( op == "and" )
    {
        ++index;
        while ( tokens[index] != ")" )
        {
            index = parseCondition( index + 1, tokens, equations );
        }

        return index + 1;
    }

    ++index;
    ASSERT( tokens[index] == ")" )
    return index + 1;
}

int VnnLibParser::parseTerm( int index, const Vector<String> &tokens, Term &term )
{
    String token = tokens[index];

    if ( token == "(" )
    {
        token = tokens[++index];
        if ( token == "+" )
        {
            term._type = Term::TermType::ADD;
        }
        else if ( token == "-" )
        {
            term._type = Term::TermType::SUB;
        }
        else if ( token == "*" )
        {
            term._type = Term::TermType::MUL;
        }
        index = parseComplexTerm( index + 1, tokens, term );
    }
    else if ( _varMap.exists( token ) )
    {
        term._type = Term::TermType::VARIABLE;
        term._value = token;
    }
    else
    {
        term._type = Term::TermType::CONST;
        term._value = token;
    }

    return index;
}

int VnnLibParser::parseComplexTerm( int index,
                                    const Vector<String> &tokens,
                                    VnnLibParser::Term &term )
{
    while ( tokens[index] != ")" )
    {
        Term arg;
        index = parseTerm( index, tokens, arg );
        term._args.append( arg );
        ++index;
    }

    return index;
}

double
VnnLibParser::processAddConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs )
{
    ASSERT( term._type == Term::TermType::ADD )

    double scalar = 0;
    double coefficient = isRhs ? -1 : 1;

    for ( const auto &arg : term._args )
    {
        if ( arg._type == Term::TermType::CONST )
        {
            scalar -= coefficient * extractScalar( arg._value );
        }
        else if ( arg._type == Term::TermType::VARIABLE )
        {
            equation.addAddend( coefficient, _varMap[arg._value] );
        }
        else if ( arg._type == Term::TermType::SUB )
        {
            if ( arg._args.size() == 2 )
            {
                throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                        "Using VNN-LIB operator '-' as a sub-term of '+' is "
                                        "allowed with only one argument" );
            }

            const Term &subArg = arg._args.first();
            if ( subArg._type == Term::TermType::CONST )
            {
                scalar += coefficient * extractScalar( subArg._value );
            }
            else if ( subArg._type == Term::TermType::VARIABLE )
            {
                equation.addAddend( -coefficient, _varMap[subArg._value] );
            }
            else
            {
                throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                        "Unsupported argument for VNN-LIB operator '+'" );
            }
        }
        else
        {
            throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                    "Unsupported argument for VNN-LIB operator '+'" );
        }
    }

    return scalar;
}

double
VnnLibParser::processSubConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs )
{
    ASSERT( term._type == Term::TermType::SUB )

    if ( term._args.empty() || term._args.size() > 2 )
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                "'-' VNN-LIB operation supports only one or two arguments" );
    }

    double scalar = 0;
    double coefficient = isRhs ? 1 : -1;

    const Term &subArg2 = term._args.last();

    if ( subArg2._type == Term::TermType::CONST )
    {
        scalar -= coefficient * extractScalar( subArg2._value );
    }
    else if ( subArg2._type == Term::TermType::VARIABLE )
    {
        equation.addAddend( coefficient, _varMap[subArg2._value] );
    }
    else
    {
        throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                "Unsupported argument for VNN-LIB operator '-'" );
    }

    if ( term._args.size() == 2 )
    {
        const Term &subArg1 = term._args.first();

        if ( subArg1._type == Term::TermType::CONST )
        {
            scalar += coefficient * extractScalar( subArg1._value );
        }
        else if ( subArg1._type == Term::TermType::VARIABLE )
        {
            equation.addAddend( -coefficient, _varMap[subArg1._value] );
        }
        else
        {
            throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                    "Unsupported argument for VNN-LIB operator '-'" );
        }
    }

    return scalar;
}

double
VnnLibParser::processMulConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs )
{
    ASSERT( term._type == Term::TermType::MUL )

    double scalar = 1;

    bool varExists = false;
    unsigned int var;

    for ( const Term &arg : term._args )
    {
        if ( arg._type == Term::TermType::CONST )
        {
            scalar *= extractScalar( arg._value );
        }
        else if ( arg._type == Term::TermType::VARIABLE )
        {
            if ( varExists )
            {
                throw InputParserError(
                    InputParserError::UNEXPECTED_INPUT,
                    "No support for using VNN-LIB operator '*' on more than one variable" );
            }

            varExists = true;
            var = _varMap[arg._value];
        }
        else if ( arg._type == Term::TermType::SUB )
        {
            if ( arg._args.size() == 2 )
            {
                throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                        "Using VNN-LIB operator '-' as a sub-term of '*' is "
                                        "allowed with only one argument" );
            }

            const Term &subArg = arg._args.first();
            if ( subArg._type == Term::TermType::CONST )
            {
                scalar *= ( -extractScalar( subArg._value ) );
            }
            else if ( subArg._type == Term::TermType::VARIABLE )
            {
                if ( varExists )
                {
                    throw InputParserError(
                        InputParserError::UNEXPECTED_INPUT,
                        "No support for using VNN-LIB operator '*' on more than one variable" );
                }

                varExists = true;
                var = _varMap[subArg._value];
                scalar *= -1;
            }
            else
            {
                throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                        "Unsupported argument for VNN-LIB operator '*'" );
            }
        }
        else
        {
            throw InputParserError( InputParserError::UNEXPECTED_INPUT,
                                    "Unsupported argument for VNN-LIB operator '*'" );
        }
    }

    double coefficient = isRhs ? -1 : 1;

    if ( varExists )
    {
        equation.addAddend( coefficient * scalar, var );
        return 0;
    }

    return ( -coefficient ) * scalar;
}

Equation VnnLibParser::processLeConstraint( const VnnLibParser::Term &arg1,
                                            const VnnLibParser::Term &arg2 )
{
    Equation equation( Equation::EquationType::LE );
    double scalar = 0;

    // Handle lhs argument:
    if ( arg1._type == Term::TermType::CONST )
    {
        scalar -= extractScalar( arg1._value );
    }
    else if ( arg1._type == Term::TermType::VARIABLE )
    {
        equation.addAddend( 1, _varMap[arg1._value] );
    }
    else if ( arg1._type == Term::TermType::ADD )
    {
        scalar += processAddConstraint( arg1, equation );
    }
    else if ( arg1._type == Term::TermType::SUB )
    {
        scalar += processSubConstraint( arg1, equation );
    }
    else if ( arg1._type == Term::TermType::MUL )
    {
        scalar += processMulConstraint( arg1, equation );
    }

    // Handle rhs argument:
    if ( arg2._type == Term::TermType::CONST )
    {
        scalar += extractScalar( arg2._value );
    }
    else if ( arg2._type == Term::TermType::VARIABLE )
    {
        equation.addAddend( -1, _varMap[arg2._value] );
    }
    else if ( arg2._type == Term::TermType::ADD )
    {
        scalar += processAddConstraint( arg2, equation, true );
    }
    else if ( arg2._type == Term::TermType::SUB )
    {
        scalar += processSubConstraint( arg2, equation, true );
    }
    else if ( arg2._type == Term::TermType::MUL )
    {
        scalar += processMulConstraint( arg2, equation, true );
    }

    equation.setScalar( scalar );
    return equation;
}
