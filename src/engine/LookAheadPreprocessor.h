/*********************                                                        */
/*! \file LookAheadPreprocessor.h
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

#include <PiecewiseLinearCaseSplit.h>

#include <Map.h>

#include <boost/lockfree/queue.hpp>

#ifndef __LookAheadPreprocessor_h__
#define __LookAheadPreprocessor_h__

class LookAheadPreprocessor
{
public:
    // Synchronized Queue containing the Sub-Queries shared by workers
    typedef boost::lockfree::queue<List<PiecewiseLinearCaseSplit> *,
        boost::lockfree::fixed_sized<false>>WorkerQueue;

    LookAheadPreprocessor();

    void run( Map<unsigned, unsigned> &bToPhase );


private:

    /*
      Set of subQueries to be solved by workers
    */
    WorkerQueue *_workload;

    void createEngines();
};

#endif

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
