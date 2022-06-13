/*********************                                                        */
/*! \file QuadraticEquation.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "QuadraticEquation.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "Map.h"

QuadraticEquation::Addend::Addend( double coefficient, unsigned variable )
    : _coefficient( coefficient )
    , _variables( { variable } )
{
}

QuadraticEquation::Addend::Addend( double coefficient, unsigned variable1,
                                   unsigned variable2 )
    : _coefficient( coefficient )
    , _variables( { variable1, variable2 } )
{
}

bool QuadraticEquation::Addend::operator==( const Addend &other ) const
{
    if ( _coefficient != other._coefficient ) return false;
    if ( _variables.size() != other._variables.size() ) return false;
    for ( unsigned i = 0; i < _variables.size(); ++i )
        if ( _variables[i] != other._variables[i] )
            return false;
    return true;
}

QuadraticEquation::QuadraticEquation()
    : _scalar( 0 )
    , _type( QuadraticEquation::EQ )
{
}

QuadraticEquation::QuadraticEquation( QuadraticEquationType type )
    : _scalar( 0 )
    , _type( type )
{
}

void QuadraticEquation::addAddend( double coefficient, unsigned variable )
{
    _addends.append( Addend( coefficient, variable ) );
}

void QuadraticEquation::addQuadraticAddend( double coefficient, unsigned variable1,
                                            unsigned variable2 )
{
    _addends.append( Addend( coefficient, variable1, variable2 ) );
}

void QuadraticEquation::setScalar( double scalar )
{
    _scalar = scalar;
}

void QuadraticEquation::setType( QuadraticEquationType type )
{
    _type = type;
}

void QuadraticEquation::updateVariableIndex( unsigned oldVar, unsigned newVar )
{
    for ( auto &addend : _addends )
    {
        for ( unsigned i = 0; i < addend._variables.size(); ++i )
            if ( addend._variables[i] == oldVar )
                addend._variables[i] = newVar;
    }
}

bool QuadraticEquation::operator==( const QuadraticEquation &other ) const
{
    return
        ( _addends == other._addends ) &&
        ( _scalar == other._scalar ) &&
        ( _type == other._type );
}

void QuadraticEquation::dump() const
{
    String output;
    dump( output );
    printf( "%s", output.ascii() );
}

void QuadraticEquation::dump( String &output ) const
{
    output = "";
    for ( const auto &addend : _addends )
    {
        if ( FloatUtils::isZero( addend._coefficient ) )
            continue;

        if ( FloatUtils::isPositive( addend._coefficient ) )
            output += String( "+" );

        if ( addend._variables.size() == 1)
            output += Stringf( "%.2lfx%u ", addend._coefficient, addend._variables[0]);
        else if ( addend._variables.size() == 2)
            output += Stringf( "%.2lfx%ux%u ", addend._coefficient, addend._variables[0], addend._variables[1]);
    }

    switch ( _type )
        {
        case QuadraticEquation::GE:
            output += String( " >= " );
            break;

        case QuadraticEquation::LE:
            output += String( " <= " );
            break;

        case QuadraticEquation::EQ:
            output += String( " = " );
            break;
        }

    output += Stringf( "%.2lf\n", _scalar );
}

Set<unsigned> QuadraticEquation::getParticipatingVariables() const
{
    Set<unsigned> result;
    for ( const auto &addend : _addends )
        for ( const auto &var : addend._variables )
            result.insert( var );

    return result;
}

double QuadraticEquation::getCoefficient( unsigned variable ) const
{
    for ( const auto &addend : _addends )
    {
        if ( addend._variables.size() == 1 && *(addend._variables.begin()) == variable )
            return addend._coefficient;
    }

    return 0;
}

double QuadraticEquation::getCoefficient( unsigned variable1,
                                          unsigned variable2 ) const
{
    for ( const auto &addend : _addends )
    {
        if ( addend._variables.size() == 2 && *(addend._variables.begin()) == variable1
             && *(++addend._variables.begin()) == variable2 )
            return addend._coefficient;
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
