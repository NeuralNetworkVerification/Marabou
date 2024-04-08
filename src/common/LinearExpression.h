/*********************                                                        */
/*! \file LinearExpression.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __LinearExpression_h__
#define __LinearExpression_h__

#include "FloatUtils.h"
#include "MStringf.h"
#include "Map.h"

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

    double evaluate( const Map<unsigned, double> &assignment );

    bool isZero() const;

    /*
      For debugging
    */
    void dump() const;

    Map<unsigned, double> _addends; // a mapping from variable to coefficient
    double _constant;
};

#endif // __LinearExpression_h__
