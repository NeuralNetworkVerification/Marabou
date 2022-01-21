/*********************                                                        */
/*! \file LinearExpression.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __LinearExpression_h__
#define __LinearExpression_h__

#include "FloatUtils.h"
#include "Map.h"
#include "MStringf.h"

/*
   A class representing a single linear expression.
   A linear expression is interpreted as:

   sum( coefficient * variable ) + constant
*/

class LinearExpression
{
public:

    LinearExpression();
    LinearExpression( Map<unsigned, double> &addends );
    LinearExpression( Map<unsigned, double> &addends, double constant );

    bool operator==( const LinearExpression &other ) const;

    /*
      For debugging
    */
    void dump() const;

    Map<unsigned, double> _addends; // a mapping from variable to coefficient
    double _constant;

    /*
      For debugging
    */
    void dump() const
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

        output += Stringf( "%.2lf ", _constant );

        printf( "%s", output.ascii() );
    }
};

#endif // __LinearExpression_h__
