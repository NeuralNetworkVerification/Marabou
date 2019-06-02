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

#include "PiecewiseLinearCaseSplit.h"

#include <boost/lockfree/queue.hpp>
#include <utility>
#include <vector>

// Guy: In general, tuples are less self-explanatory than structs (fields are not named in tuples). Consider changing this.

// Object representing a subquery, containing a string denoting the id of the
// query, a case split containing the input ranges, and an unsigned int denoting
// the timeout

// Guy: copying strings is cheap. For simplicity, consider removing the std::unique_ptr
typedef std::tuple<std::unique_ptr<std::string>, std::unique_ptr
  <PiecewiseLinearCaseSplit>, unsigned> SubQuery;

// Synchronized Queue containing the Sub-Queries shared by workers
typedef boost::lockfree::queue<SubQuery*, boost::lockfree::
  fixed_sized<false>>WorkerQueue;

// A vector of Sub-Queries

// Guy: consider using our wrapper class Vector instead of std::vector
typedef std::vector<SubQuery*> SubQueries;

#endif // __SubQuery_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
