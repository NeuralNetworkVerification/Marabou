/*********************                                                        */
/*! \file Equation.h
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

#ifndef __Equation_h__
#define __Equation_h__

#include "List.h"
#include "MString.h"
#include "Set.h"

/*
   A class representing a single equation.
   An equation is interpreted as:

   sum( coefficient * variable ) op scalar

   Where op is either =, <= or >=
*/

class Equation
{
public:
    enum EquationType {
        EQ = 0,
        GE = 1,
        LE = 2
    };

    struct Addend
    {
    public:
        Addend( double coefficient, unsigned variable );

        double _coefficient;
        unsigned _variable;

        bool operator==( const Addend &other ) const;
    };

    Equation();
    Equation( EquationType type );

    void addAddend( double coefficient, unsigned variable );
    void setScalar( double scalar );
    void setType( EquationType type );

    /*
      Go over the addends and rename variable oldVar to newVar.
      If, as a result, there are two addends with the same variable,
      unite them.
    */
    void updateVariableIndex( unsigned oldVar, unsigned newVar );

    /*
      Return true iff the variable is a "variable merging equation",
      i.e. an equation of the form x = y. If true is returned, x1 and
      x2 are the merged variables.
    */
    bool isVariableMergingEquation( unsigned &x1, unsigned &x2 ) const;

    /*
      Get the set of indices of all variables that participate in this
      equation.
    */
    Set<unsigned> getParticipatingVariables() const;

    /*
      Retrieve the coefficient of a variable
    */
    double getCoefficient( unsigned variable ) const;

    List<Addend> _addends;
    double _scalar;
    EquationType _type;

    bool operator==( const Equation &other ) const;
    bool equivalent( const Equation &other ) const;

    void dump() const;
    void dump( String &output ) const;
};

#endif // __Equation_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
