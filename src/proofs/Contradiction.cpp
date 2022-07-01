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

Contradiction::Contradiction( unsigned var, const Vector<double> &upperBoundExplanation, const Vector<double> &lowerBoundExplanation )
    : _var( var )
{
    if ( upperBoundExplanation.empty() )
        _upperBoundExplanation = NULL;
    else
    {
        _upperBoundExplanation = new double[upperBoundExplanation.size()];
        std::copy( upperBoundExplanation.begin(), upperBoundExplanation.end(), _upperBoundExplanation );
    }

    if ( lowerBoundExplanation.empty() )
        _lowerBoundExplanation = NULL;
    else
    {
        _lowerBoundExplanation = new double[lowerBoundExplanation.size()];
        std::copy( lowerBoundExplanation.begin(), lowerBoundExplanation.end(), _lowerBoundExplanation );
    }
}

Contradiction::~Contradiction()
{
    if ( _upperBoundExplanation )
    {
        delete [] _upperBoundExplanation;
        _upperBoundExplanation = NULL;
    }

    if ( _lowerBoundExplanation )
    {
        delete [] _lowerBoundExplanation;
        _lowerBoundExplanation = NULL;
    }
}

unsigned Contradiction::getVar() const
{
    return _var;
}

const double *Contradiction::getUpperBoundExplanation() const
{
    return _upperBoundExplanation;
}

const double *Contradiction::getLowerBoundExplanation() const
{
    return _lowerBoundExplanation;
}
