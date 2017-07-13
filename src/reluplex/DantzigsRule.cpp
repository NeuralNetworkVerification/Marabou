/*********************                                                        */
/*! \file DantzigsRule.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "DantzigsRule.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "ReluplexError.h"

bool DantzigsRule::select( ITableau &tableau )
{
    tableau.computeCostFunction();

    List<unsigned> candidates;
    tableau.getCandidates(candidates);

    if ( candidates.empty() )
        return false;

    // Dantzig's rule
    const double *costFunction = tableau.getCostFunction();

    List<unsigned>::const_iterator candidate = candidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = FloatUtils::abs( costFunction[maxIndex] );
    ++candidate;

    while ( candidate != candidates.end() )
    {
        double contenderValue = FloatUtils::abs( costFunction[*candidate] );
        if ( FloatUtils::gt( contenderValue, maxValue ) )
        {
            maxIndex = *candidate;
            maxValue = contenderValue;
        }
        ++candidate;
    }

    tableau.setEnteringVariable(maxIndex);
    return true;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
