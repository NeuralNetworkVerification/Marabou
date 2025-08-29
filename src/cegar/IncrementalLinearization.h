/*********************                                                        */
/*! \file IncrementalLinearization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __IncrementalLinearization_h__
#define __IncrementalLinearization_h__

#include "MStringf.h"
#include "Map.h"
#include "Query.h"

#define INCREMENTAL_LINEARIZATION_LOG( x, ... )                                                    \
    LOG( GlobalConfiguration::CEGAR_LOGGING, "IncrementalLinearization: %s\n", x )

class Engine;
class IQuery;

namespace CEGAR {

class IncrementalLinearization
{
public:
    enum {
        MICROSECONDS_TO_SECONDS = 1000000,
    };

    IncrementalLinearization( IQuery &inputQuery, Engine *engine );

    /*
      Solve with incremental linarizations.
      Only for the purpose of Nonlinear Constraints
    */
    void solve();

    Engine *releaseEngine();

    void setInitialTimeoutInMicroSeconds( unsigned long long timeoutInMicroSeconds );

    /*
      Refine the abstraction by adding constraints to exclude
      the counter-example related to the given constraint

      Returns the number of refined non-linear constraints
    */
    unsigned refine( Query &inputQuery );

private:
    IQuery &_inputQuery;
    std::unique_ptr<Engine> _engine;

    /*
      A local copy of all the non-linear constraints in _inputQuery
    */
    Vector<NonlinearConstraint *> _nlConstraints;

    unsigned long long _timeoutInMicroSeconds;

    // How many rounds of incremental linearization have been performed
    unsigned _round;

    // Number of additional equations
    unsigned _numAdditionalEquations;
    // Number of additional piecewise-linear constraints
    unsigned _numAdditionalPLConstraints;

    // Hyper-parmeters of incremental linearization
    unsigned _numConstraintsToRefine;
    double _refinementScalingFactor;

    void printStatus();
};

} // namespace CEGAR


#endif // __IncrementalLinearization_h__
