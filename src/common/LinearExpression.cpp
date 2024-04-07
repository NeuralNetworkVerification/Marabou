/*********************                                                        */
/*! \file LinearExpression.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "LinearExpression.h"

#include "FloatUtils.h"
#include "MStringf.h"

LinearExpression::LinearExpression()
    : _constant( 0 )
{
}

LinearExpression::LinearExpression( Map<unsigned, double> &addends )
    : _addends( addends )
    , _constant( 0 )
{
}

LinearExpression::LinearExpression( Map<unsigned, double> &addends, double constant )
    : _addends( addends )
    , _constant( constant )
{
}

bool LinearExpression::operator==( const LinearExpression &other ) const
{
    return ( _addends == other._addends ) && ( _constant == other._constant );
}

double LinearExpression::evaluate( const Map<unsigned, double> &assignment )
{
    double sum = 0;
    for ( const auto &addend : _addends )
    {
        if ( FloatUtils::isZero( addend.second ) )
            continue;
        else if ( assignment.exists( addend.first ) )
            sum += addend.second * assignment[addend.first];
        else
            return NAN;
    }
    sum += _constant;
    return sum;
}

bool LinearExpression::isZero() const
{
    for ( const auto &addend : _addends )
    {
        if ( addend.second != 0 )
            return false;
    }
    return _constant == 0;
}

void LinearExpression::dump() const
{
    String output = "";
    for ( const auto &addend : _addends )
    {
        if ( FloatUtils::isZero( addend.second ) )
            continue;

        if ( FloatUtils::isPositive( addend.second ) )
            output += String( "+" );

        output += Stringf( "%.2lfx%u ", addend.second, addend.first );
    }

    if ( FloatUtils::isPositive( _constant ) )
        output += String( "+" );
    if ( !FloatUtils::isZero( _constant ) )
        output += Stringf( "%.2lf ", _constant );

    printf( "%s\n", output.ascii() );
}
