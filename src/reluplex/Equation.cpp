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

Equation::Addend::Addend( double coefficient, unsigned variable )
    : _coefficient( coefficient )
    , _variable( variable )
{
}

bool Equation::Addend::operator==( const Addend &other ) const
{
    return ( _coefficient == other._coefficient ) && ( _variable == other._variable );
}

void Equation::addAddend( double coefficient, unsigned variable )
{
    _addends.append( Addend( coefficient, variable ) );
}

void Equation::setScalar( double scalar )
{
    _scalar = scalar;
}

void Equation::markAuxiliaryVariable( unsigned auxVariable )
{
    _auxVariable = auxVariable;
}

bool Equation::operator==( const Equation &other ) const
{
    return
        ( _addends == other._addends ) &&
        ( _scalar == other._scalar ) &&
        ( _auxVariable == other._auxVariable );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
