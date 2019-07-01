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
#include "SubQuery.h"

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
                DivideStrategy divideStrategy, String networkFilePath,
                String propertyFilePath );

    ~DnCManager();

    void freeMemoryIfNeeded();

    /*
      Perform the Divide-and-conquer solving
    */
    void solve();

    /*
      Return the DnCExitCode of the DnCManager
    */
    DnCExitCode getExitCode() const;

private:
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
      Print the result of DnC solving
    */
    void printResult();

    /*
      The base engine that is used to perform the initial divides
    */
    std::shared_ptr<Engine> _baseEngine;

    /*
      The engines that are run in different threads
    */
    Vector<std::shared_ptr<Engine>> _engines;

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
      Path to the network and property files
    */
    String _networkFilePath;
    String _propertyFilePath;

    /*
      The exit code of the DnCManager.
    */
    DnCExitCode _exitCode;

    /*
      Set of subQueries to be solved by workers
    */
    WorkerQueue *_workload;
};

#endif // __DnCManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
