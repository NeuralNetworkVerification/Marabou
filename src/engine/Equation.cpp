/*********************                                                        */
/*! \file Equation.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Equation.h"
#include "FloatUtils.h"

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
    : _type( Equation::EQ )
{
}

Equation::Equation( EquationType type )
    : _type( type )
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

void Equation::dump() const
{
    for ( const auto &addend : _addends )
    {
        if ( FloatUtils::isZero( addend._coefficient ) )
            continue;

        if ( FloatUtils::isPositive( addend._coefficient ) )
            printf( "+" );

        printf( "%.2lfx%u ", addend._coefficient, addend._variable );
    }

    switch ( _type )
    {
    case Equation::GE:
        printf( " >= " );
        break;

    case Equation::LE:
        printf( " <= " );
        break;

    case Equation::EQ:
        printf( " = " );
        break;
    }

    printf( "%.2lf\n", _scalar );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
