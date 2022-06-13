/*********************                                                        */
/*! \file QuadraticEquation.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __QuadraticEquation_h__
#define __QuadraticEquation_h__

#include "List.h"
#include "MString.h"
#include "Set.h"
#include "Vector.h"

/*
   A class representing a single equation.
   An equation is interpreted as:

   sum( coefficient * variable ) op scalar

   Where op is either =, <= or >=
*/

class QuadraticEquation
{
public:
    enum QuadraticEquationType {
        EQ = 0,
        GE = 1,
        LE = 2
    };

    struct Addend
    {
    public:
        Addend( double coefficient, unsigned variable );
        Addend( double coefficient, unsigned variable1, unsigned variable2 );

        double _coefficient;
        Vector<unsigned> _variables;

        bool operator==( const Addend &other ) const;
    };

    QuadraticEquation();
    QuadraticEquation( QuadraticEquationType type );

    void addAddend( double coefficient, unsigned variable );
    void addQuadraticAddend( double coefficient, unsigned variable1, unsigned variable2 );
    void setScalar( double scalar );
    void setType( QuadraticEquationType type );

    /*
      Go over the addends and rename variable oldVar to newVar.
      If, as a result, there are two addends with the same variable,
      unite them.
    */
    void updateVariableIndex( unsigned oldVar, unsigned newVar );

    /*
      Get the set of indices of all variables that participate in this
      equation.
    */
    Set<unsigned> getParticipatingVariables() const;

    /*
      Retrieve the coefficient of a term
    */
    double getCoefficient( unsigned variable ) const;

    /*
      Retrieve the coefficient of a term
    */
    double getCoefficient( unsigned variable1, unsigned variable2 ) const;


    List<Addend> _addends;
    double _scalar;
    QuadraticEquationType _type;

    bool operator==( const QuadraticEquation &other ) const;

    void dump() const;
    void dump( String &output ) const;
};

#endif // __QuadraticEquation_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
