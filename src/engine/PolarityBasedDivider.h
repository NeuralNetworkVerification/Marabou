/*********************                                                        */
/*! \file PolarityBasedDivider.h
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

#ifndef __PolarityBasedDivider_h__
#define __PolarityBasedDivider_h__

#include "List.h"
#include "QueryDivider.h"

#include <math.h>

class PolarityBasedDivider : public QueryDivider
{
public:
    PolarityBasedDivider( std::shared_ptr<IEngine> engine );

    void createSubQueries( unsigned numNewSubQueries,
                           const String queryIdPrefix,
                           const PiecewiseLinearCaseSplit
                           &previousSplit,
                           const unsigned timeoutInSeconds,
                           SubQueries &subQueries );

private:
    std::shared_ptr<IEngine> _engine;

    /*
      Returns the variable with the largest range
    */
    PiecewiseLinearConstraint *getPLConstraintToSplit( const
                                                       PiecewiseLinearCaseSplit
                                                       &split );
};

#endif // __LargestIntervalDivider_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
