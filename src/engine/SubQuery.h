/*********************                                                        */
/*! \file SubQuery.h
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

#ifndef __SubQuery_h__
#define __SubQuery_h__

#include "List.h"
#include "PiecewiseLinearCaseSplit.h"

#include <boost/lockfree/queue.hpp>
#include <utility>

// Struct representing a subquery
struct SubQuery {
    std::string queryId;
    std::unique_ptr<PiecewiseLinearCaseSplit> split;
    unsigned timeoutInSeconds;
};

// Synchronized Queue containing the Sub-Queries shared by workers
typedef boost::lockfree::queue<SubQuery*, boost::lockfree::
  fixed_sized<false>>WorkerQueue;

// A vector of Sub-Queries

// Guy: consider using our wrapper class Vector instead of std::vector
typedef List<SubQuery*> SubQueries;

#endif // __SubQuery_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
