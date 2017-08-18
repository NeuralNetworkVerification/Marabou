/*********************                                                        */
/*! \file BlandsRule.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BlandsRule.h"
#include "ITableau.h"
#include "ReluplexError.h"

bool BlandsRule::select( ITableau &tableau )
{
    List<unsigned> candidates;
    tableau.getEntryCandidates( candidates );

    if ( candidates.empty() )
        return false;

    List<unsigned>::const_iterator it = candidates.begin();
    unsigned minIndex = *it;
    unsigned minVariable = tableau.nonBasicIndexToVariable( minIndex );

    ++it;
    unsigned variable;
    while ( it != candidates.end() )
    {
        variable = tableau.nonBasicIndexToVariable( *it );
        if ( variable < minVariable )
        {
            minIndex = *it;
            minVariable = variable;
        }

        ++it;
    }

    tableau.setEnteringVariable( minIndex );
    return true;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
