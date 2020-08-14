/*********************                                                        */
/*! \file DnCWorker.h
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

#ifndef __DnCWorker_h__
#define __DnCWorker_h__

#include "DivideStrategy.h"
#include "Engine.h"
#include "PiecewiseLinearCaseSplit.h"
#include "QueryDivider.h"

#include <atomic>

class DnCWorker
{
public:
    DnCWorker( WorkerQueue *workload, std::shared_ptr<IEngine> engine,
               std::atomic_uint &numUnsolvedSubqueries,
               std::atomic_bool &shouldQuitSolving, unsigned threadId,
               unsigned onlineDivides, float timeoutFactor,
               DivideStrategy divideStrategy, unsigned verbosity );

    /*
      Pop one subQuery, solve it and handle the result
      Return true if the DnCWorker should continue running
    */
    void popOneSubQueryAndSolve( bool restoreTreeStates=true );

private:
    /*
      Initiate the query-divider object
    */
    void setQueryDivider( DivideStrategy divideStrategy );

    /*
      Convert the exitCode to string
    */
    static String exitCodeToString( IEngine::ExitCode result );

    /*
      Print the current progress
    */
    void printProgress( String queryId, IEngine::ExitCode result ) const;

    /*
      The queue of subqueries (shared across threads)
    */
    WorkerQueue *_workload;
    std::shared_ptr<IEngine> _engine;

    /*
      The number of unsolved subqueries
    */
    std::atomic_uint *_numUnsolvedSubQueries;

    /*
      A boolean denoting whether a solution has been found
    */
    std::atomic_bool *_shouldQuitSolving;
    std::unique_ptr<QueryDivider> _queryDivider;

    /*
      Initial state of the engine to which engine is restored after handling
      a subquery
    */
    std::shared_ptr<EngineState> _initialState;

    unsigned _threadId;
    unsigned _onlineDivides;
    float _timeoutFactor;
    unsigned _verbosity;
};

#endif // __DnCWorker_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
