/*********************                                                        */
/*! \file DnCManager.h
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

#ifndef __DnCManager_h__
#define __DnCManager_h__

#include "DivideStrategy.h"
#include "Engine.h"
#include "InputQuery.h"
#include "SubQuery.h"
#include "Vector.h"

#include <atomic>

class DnCManager
{
public:

    enum DnCExitCode
        {
            UNSAT = 0,
            SAT = 1,
            ERROR = 2,
            TIMEOUT = 3,
            QUIT_REQUESTED = 4,

            NOT_DONE = 999,
        };

    DnCManager( unsigned numWorkers, unsigned initialDivides, unsigned
                initialTimeout, unsigned onlineDivides, float timeoutFactor,
                DivideStrategy divideStrategy, InputQuery *inputQuery,
                unsigned verbosity );

    ~DnCManager();

    void freeMemoryIfNeeded();

    /*
      Perform the Divide-and-conquer solving
    */
    void solve( unsigned timeoutInSeconds );

    /*
      Return the DnCExitCode of the DnCManager
    */
    DnCExitCode getExitCode() const;

    /*
      Get the string representation of the exitcode
    */
    String getResultString();

    /*
      Print the result of DnC solving
    */
    void printResult();

    /*
      Store the solution into the map
    */
    void getSolution( std::map<int, double> &ret );

    void setConstraintViolationThreshold( unsigned threshold );

private:
    /*
      Create and run a DnCWorker
    */
    static void dncSolve( WorkerQueue *workload, std::shared_ptr<Engine> engine,
                          std::unique_ptr<InputQuery> inputQuery,
                          std::atomic_uint &numUnsolvedSubQueries,
                          std::atomic_bool &shouldQuitSolving,
                          unsigned threadId, unsigned onlineDivides,
                          float timeoutFactor, DivideStrategy divideStrategy );

    /*
      Create the base engine from the network and property files,
      and if necessary, create engines for workers
    */
    bool createEngines();

    /*
      Divide up the input region and store them in subqueries
    */
    void initialDivide( SubQueries &subQueries );

    /*
      Read the exitCode of the engine of each thread, and update the manager's
      exitCode.
    */
    void updateDnCExitCode();

    /*
      Set _timeoutReached to true if timeout has been reached
    */
    void updateTimeoutReached( timespec startTime,
                               unsigned long long timeoutInMicroSeconds );

    static void log( const String &message );

    /*
      The base engine that is used to perform the initial divides
    */
    std::shared_ptr<Engine> _baseEngine;

    /*
      The engines that are run in different threads
    */
    Vector<std::shared_ptr<Engine>> _engines;

    /*
      The engine with the satisfying assignment
    */
    std::shared_ptr<Engine> _engineWithSATAssignment;

    /*
      Hyperparameters of the DnC algorithm
    */

    /*
      The number of threads to spawn
    */
    unsigned _numWorkers;

    /*
      The number of times to initially bisect the input region
    */
    unsigned _initialDivides;

    /*
      The timeout given to the initial subqueries
    */
    unsigned _initialTimeout;

    /*
      The number of times to bisect the sub-input-region if a subquery times out
    */
    unsigned _onlineDivides;

    /*
      When a subQuery is created from dividing a query, the new timeout is the
      old timeout times this factor
    */
    float _timeoutFactor;

    /*
      The strategy for dividing a query
    */
    DivideStrategy _divideStrategy;

    /*
      Alternatively, we could construct the DnCManager by directly providing the
      inputQuery instead of the network and property filepaths.
    */
    InputQuery *_baseInputQuery;

    /*
      The exit code of the DnCManager.
    */
    DnCExitCode _exitCode;

    /*
      Set of subQueries to be solved by workers
    */
    WorkerQueue *_workload;

    /*
      Whether the timeout has been reached
    */
    bool _timeoutReached;

    /*
      The number of currently unsolved sub queries
    */
    std::atomic_uint _numUnsolvedSubQueries;

    /*
      The level of verbosity
    */
    unsigned _verbosity;

    /*
      The constraint violation threshold for each worker engine
    */
    unsigned _constraintViolationThreshold;

};

#endif // __DnCManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
