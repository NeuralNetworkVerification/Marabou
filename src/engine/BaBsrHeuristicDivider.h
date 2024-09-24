/*********************                                                        */
/*! \file BaBsrHeuristicDivider.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Liam Davis
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __BaBsrHeuristicDivider_h__
#define __BaBsrHeuristicDivider_h__

#include "List.h"
#include "QueryDivider.h"

#include <math.h>

class BaBsrHeuristicDivider : public QueryDivider
{
public:
    BaBsrHeuristicDivider( const List<unsigned> &inputVariables );

    void createSubQueries( unsigned numNewSubQueries,
                           const String queryIdPrefix,
                           const unsigned previousDepth,
                           const PiecewiseLinearCaseSplit &previousSplit,
                           const unsigned timeoutInSeconds,
                           SubQueries &subQueries );

    /*
      Returns the variable based on the BaBSR heuristic
    */
    unsigned getNodeToSplit( const InputRegion &inputRegion );

private:
    /*
      All input variables of the network
    */
    const List<unsigned> _inputVariables;

    /*
      Calculate the heuristic score for a given node
    */
    unsigned calculateHeuristicScore( unsigned node, double upper_bound, double lower_bound );
};

#endif // __BaBsrHeuristicDivider_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End: