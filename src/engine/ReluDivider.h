/*********************                                                        */
/*! \file ReluLookAheadDivider.h
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

#ifndef __ReluDivider_h__
#define __ReluDivider_h__

#include "IEngine.h"
#include "List.h"
#include "PiecewiseLinearConstraint.h"
#include "QueryDivider.h"

#include <math.h>

class ReluDivider : public QueryDivider
{
public:
    ReluDivider( std::shared_ptr<IEngine> engine );

    void createSubQueries( unsigned numNewSubQueries,
                           const String queryIdPrefix,
                           const PiecewiseLinearCaseSplit
                           &previousSplit,
                           const unsigned timeoutInSeconds,
                           SubQueries &subQueries );

    /*
      Returns the variable with the largest range
    */
    PiecewiseLinearConstraint *getPLConstraintToSplit( const
                                                       PiecewiseLinearCaseSplit
                                                       &split );

private:
    /*
      The engine
    */
    std::shared_ptr<IEngine> _engine;

    unsigned _threshold;
};

#endif // __ReluDivider_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
