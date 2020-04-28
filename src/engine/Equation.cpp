/*********************                                                        */
/*! \file Equation.cpp
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

#include "Equation.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "Map.h"

Equation::Addend::Addend( double coefficient, unsigned variable )
    : _coefficient( coefficient )
    , _variable( variable )
{
}

bool Equation::Addend::operator==( const Addend &other ) const
{
    return ( _coefficient == other._coefficient ) && ( _variable == other._variable );
}

Equation::Equation()
    : _scalar( 0 )
    , _type( Equation::EQ )
{
}

Equation::Equation( EquationType type )
    : _scalar( 0 )
    , _type( type )
{
}

void Equation::addAddend( double coefficient, unsigned variable )
{
    _addends.append( Addend( coefficient, variable ) );
}

void Equation::setScalar( double scalar )
{
    _scalar = scalar;
}

void Equation::setType( EquationType type )
{
    _type = type;
}

void Equation::updateVariableIndex( unsigned oldVar, unsigned newVar )
{
    // Find oldVar's addend and update it
    List<Addend>::iterator oldVarIt = _addends.begin();
    while ( oldVarIt != _addends.end() && oldVarIt->_variable != oldVar )
        ++oldVarIt;

    // OldVar doesn't exist - can stop
    if ( oldVarIt == _addends.end() )
        return;

    // Update oldVar's index
    oldVarIt->_variable = newVar;

    // Check to see if there are now two addends for newVar. If so,
    // remove one and adjust the coefficient
    List<Addend>::iterator newVarIt;
    for ( newVarIt = _addends.begin(); newVarIt != _addends.end(); ++newVarIt )
    {
        if ( newVarIt == oldVarIt )
            continue;

        if ( newVarIt->_variable == newVar )
        {
            oldVarIt->_coefficient += newVarIt->_coefficient;
            _addends.erase( newVarIt );
            return;
        }
    }
}

bool Equation::operator==( const Equation &other ) const
{
    return
        ( _addends == other._addends ) &&
        ( _scalar == other._scalar ) &&
        ( _type == other._type );
}

bool Equation::equivalent( const Equation &other ) const
{
    if ( _scalar != other._scalar )
        return false;

    if ( _type != other._type )
        return false;

    Map<unsigned, double> us;
    Map<unsigned, double> them;

    for ( const auto &addend : _addends )
        us[addend._variable] = addend._coefficient;

    for ( const auto &addend : other._addends )
        them[addend._variable] = addend._coefficient;

    return us == them;
}

void Equation::dump() const
{
    String output;
    dump( output );
    printf( "%s", output.ascii() );
}

void Equation::dump( String &output ) const
{
    output = "";
    for ( const auto &addend : _addends )
    {
        if ( FloatUtils::isZero( addend._coefficient ) )
            continue;

        if ( FloatUtils::isPositive( addend._coefficient ) )
            output += String( "+" );

        output += Stringf( "%.2lfx%u ", addend._coefficient, addend._variable );
    }

    switch ( _type )
    {
    case Equation::GE:
        output += String( " >= " );
        break;

    case Equation::LE:
        output += String( " <= " );
        break;

    case Equation::EQ:
        output += String( " = " );
        break;
    }

    output += Stringf( "%.2lf\n", _scalar );
}

bool Equation::isVariableMergingEquation( unsigned &x1, unsigned &x2 ) const
{
    if ( _addends.size() != 2 )
        return false;

    if ( _type != Equation::EQ )
        return false;

    if ( !FloatUtils::isZero( _scalar ) )
        return false;

    double coefficientOne = _addends.front()._coefficient;
    double coefficientTwo = _addends.back()._coefficient;

    if ( FloatUtils::isZero( coefficientOne ) || FloatUtils::isZero( coefficientTwo ) )
        return false;

    if ( FloatUtils::areEqual( coefficientOne, -coefficientTwo ) )
    {
        x1 = _addends.front()._variable;
        x2 = _addends.back()._variable;
        return true;
    }

    return false;
}

Set<unsigned> Equation::getParticipatingVariables() const
{
    Set<unsigned> result;
    for ( const auto &addend : _addends )
        result.insert( addend._variable );

    return result;
}

double Equation::getCoefficient( unsigned variable ) const
{
    for ( const auto &addend : _addends )
    {
        if ( addend._variable == variable )
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
