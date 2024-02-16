/*********************                                                        */
/*! \file Contradiction.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "Contradiction.h"

Contradiction::Contradiction( const Vector<double> &contradiction )
{
    if ( contradiction.empty() )
        _contradiction.initializeToEmpty();
    else
        _contradiction.initialize( contradiction.data(), contradiction.size() );
}

Contradiction::Contradiction( unsigned var )
    : _var( var )
    , _contradiction()
{
}

Contradiction::~Contradiction()
{
    if ( !_contradiction.empty() )
        _contradiction.clear();
}

unsigned Contradiction::getVar() const
{
    return _var;
}

const SparseUnsortedList &Contradiction::getContradiction() const
{
    return _contradiction;
}
