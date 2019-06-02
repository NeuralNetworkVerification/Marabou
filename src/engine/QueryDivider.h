/*********************                                                        */
/*! \file QueryDivider.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __QueryDivider_h__
#define __QueryDivider_h__

#include "List.h"
#include "Map.h"
#include "SubQuery.h"

class QueryDivider
{
public:
    struct InputRegion
    {
        Map<unsigned, double> _lowerBounds;
        Map<unsigned, double> _upperBounds;
    };

    /*
      Divide the previousSubquery into |numNewSubQueries| new subqueries and
      store them in subqueries
    */
    virtual void createSubQueries( unsigned numNewSubQueries,
                                   const SubQuery &previousSubQuery,
                                   SubQueries &subQueries ) = 0;

    /*
      Bisect the given input region at the given dimension, and store the
      new input regions into inputRegions
    */
    void bisectInputRegion( const InputRegion &inputRegion,
                            unsigned dimensionToBisect,
                            List<InputRegion> &inputRegions );
};

#endif // __Querydivider_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
