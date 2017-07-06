/*********************                                                        */
/*! \file Equation.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Equation_h__
#define __Equation_h__

#include "List.h"

/* A class representing a single input equation. For now, all
   equations are interpreted as equalities, i.e. the sum of all
   addends equals the scalar */
struct Equation
{
public:
    struct Addend
    {
    public:
        Addend( double coefficient, unsigned variable );

        double _coefficient;
        unsigned _variable;

        bool operator==( const Addend &other ) const;
    };

    void addAddend( double coefficient, unsigned variable );
    void setScalar( double scalar );
    void markAuxiliaryVariable( unsigned auxVariable );

    List<Addend> _addends;
    double _scalar;
    unsigned _auxVariable;

    bool operator==( const Equation &other ) const;
};

#endif // __Equation_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
