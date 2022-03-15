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

#include <cstddef>
#include "Contradiction.h"

Contradiction::Contradiction( unsigned var, double *upperBoundExplanation, double *lowerBoundExplanation )
        :_var( var )
        ,_upperBoundExplanation( upperBoundExplanation )
        ,_lowerBoundExplanation( lowerBoundExplanation )
{
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