/*********************                                                        */
/*! \file BlandsRule.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "BlandsRule.h"
#include "ITableau.h"
#include "ReluplexError.h"

bool BlandsRule::select( ITableau &tableau,
                         const List<unsigned> &candidates,
                         const Set<unsigned> &excluded )
{
    List<unsigned> remainingCandidates = candidates;

    List<unsigned>::iterator it = remainingCandidates.begin();
    while ( it != remainingCandidates.end() )
    {
        if ( excluded.exists( *it ) )
            it = remainingCandidates.erase( it );
        else
            ++it;
    }

    if ( remainingCandidates.empty() )
        return false;

    it = remainingCandidates.begin();
    unsigned minIndex = *it;
    unsigned minVariable = tableau.nonBasicIndexToVariable( minIndex );

    ++it;
    unsigned variable;
    while ( it != remainingCandidates.end() )
    {
        variable = tableau.nonBasicIndexToVariable( *it );
        if ( variable < minVariable )
        {
            minIndex = *it;
            minVariable = variable;
        }

        ++it;
    }

    tableau.setEnteringVariableIndex( minIndex );
    return true;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
