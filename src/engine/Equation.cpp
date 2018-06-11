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

void Equation::updateVariableIndex( unsigned v1, unsigned v2 )
{
    bool alreadyV2 = false;
    bool alreadyV1 = false;
    Addend toRemove( 0, 0 );
    for ( auto addend: _addends )
    {
        if ( addend._variable == v1 )
            alreadyV1 = true;
        if ( addend._variable == v2 ){
            alreadyV2 = true;
            toRemove = addend;
        }
    }
    if ( !alreadyV1 )
        return;
    double oldCoeff = 0;
    if ( alreadyV2 ){
        _addends.erase( toRemove );
        oldCoeff = toRemove._coefficient;
    }
    for (auto &addend: _addends )
    {
        if ( addend._variable == v1 ){
            addend._variable = v2;
            addend._coefficient += oldCoeff;
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
