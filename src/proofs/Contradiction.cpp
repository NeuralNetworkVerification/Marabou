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

Contradiction::Contradiction( const Vector<double> &contradictionVec )
{
    if ( contradictionVec.empty() )
        _contradictionVec = NULL;
    else
    {
        _contradictionVec = new double[contradictionVec.size()];
        std::copy( contradictionVec.begin(), contradictionVec.end(), _contradictionVec );
    }
}

Contradiction::Contradiction( unsigned var )
    : _var( var )
    , _contradictionVec( NULL )
{
}

Contradiction::~Contradiction()
{
    if ( _contradictionVec )
    {
        delete [] _contradictionVec;
        _contradictionVec = NULL;
    }
}

unsigned Contradiction::getVar() const
{
    return _var;
}

const double *Contradiction::getContradictionVec() const
{
    return _contradictionVec;
}
