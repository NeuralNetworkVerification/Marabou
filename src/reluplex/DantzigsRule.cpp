/*********************                                                        */
/*! \file DantzigsRule.h
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

unsigned DantzigsRule::select( const List<unsigned> &candidates, const ITableau &tableau )
{
    if ( candidates.empty() )
        throw ReluplexError( ReluplexError::NO_AVAILABLE_CANDIDATES, "DantzigsRule" );

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

    return maxIndex;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
