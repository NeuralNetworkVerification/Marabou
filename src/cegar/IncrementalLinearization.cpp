/*********************                                                        */
/*! \file IncrementalLinearizatoin.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "IncrementalLinearization.h"

#include "Engine.h"
#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "IQuery.h"
#include "Options.h"
#include "Query.h"
#include "TimeUtils.h"

#include <random>

namespace CEGAR {

IncrementalLinearization::IncrementalLinearization( IQuery &inputQuery, Engine *engine )
    : _inputQuery( inputQuery )
    , _engine( std::unique_ptr<Engine>( engine ) )
    , _timeoutInMicroSeconds( 0 )
    , _round( 0 )
    , _numAdditionalEquations( 0 )
    , _numAdditionalPLConstraints( 0 )
    , _numConstraintsToRefine(
          Options::get()->getInt( Options::NUM_CONSTRAINTS_TO_REFINE_INC_LIN ) )
    , _refinementScalingFactor(
          Options::get()->getFloat( Options::REFINEMENT_SCALING_FACTOR_INC_LIN ) )
{
    srand( Options::get()->getInt( Options::SEED ) );
    _inputQuery.getNonlinearConstraints( _nlConstraints );
}

void IncrementalLinearization::solve()
{
    /*
      Invariants at the beginning of the loop:
      1. _inputQuery contains the assignment in the previous refinement round
      2. _timeoutInMicroSeconds is positive
    */
    while ( true )
    {
        struct timespec start = TimeUtils::sampleMicro();

        // Refine the non-linear constraints using the counter-example stored
        // in the _inputQuery
        Query refinement;
        refinement.setNumberOfVariables( _inputQuery.getNumberOfVariables() );
        _engine->extractSolution( refinement );
        _engine->extractBounds( refinement );
        unsigned numRefined = refine( refinement );

        printStatus();
        if ( numRefined == 0 )
            return;

        // Create a new engine
        _engine = std::unique_ptr<Engine>( new Engine() );
        _engine->setVerbosity( 0 );

        // Solve the refined abstraction
        if ( _engine->processInputQuery( _inputQuery ) )
        {
            double timeoutInSeconds =
                static_cast<long double>( _timeoutInMicroSeconds ) / MICROSECONDS_TO_SECONDS;
            INCREMENTAL_LINEARIZATION_LOG(
                Stringf( "Solving with timeout %.2f seconds", timeoutInSeconds ).ascii() );
            _engine->solve( timeoutInSeconds );
        }

        if ( _engine->getExitCode() == IEngine::UNKNOWN )
        {
            unsigned long long timePassed =
                TimeUtils::timePassed( start, TimeUtils::sampleMicro() );
            if ( timePassed >= _timeoutInMicroSeconds )
            {
                // Enter another round but should quit immediately.
                _timeoutInMicroSeconds = 1;
            }
            else
                _timeoutInMicroSeconds -= TimeUtils::timePassed( start, TimeUtils::sampleMicro() );
            _numConstraintsToRefine =
                std::min( _nlConstraints.size(),
                          (unsigned)( _numConstraintsToRefine * _refinementScalingFactor ) );
        }
        else
            return;
    }
}

unsigned IncrementalLinearization::refine( Query &refinement )
{
    INCREMENTAL_LINEARIZATION_LOG( "Performing abstraction refinement..." );

    unsigned numRefined = 0;
    for ( const auto &nlc : _nlConstraints )
    {
        numRefined += nlc->attemptToRefine( refinement );
        if ( numRefined >= _numConstraintsToRefine )
            break;
    }
    _inputQuery.setNumberOfVariables( refinement.getNumberOfVariables() );
    for ( const auto &e : refinement.getEquations() )
        _inputQuery.addEquation( e );
    _numAdditionalEquations += refinement.getEquations().size();

    for ( const auto &plc : refinement.getPiecewiseLinearConstraints() )
    {
        _inputQuery.addPiecewiseLinearConstraint( plc );
    }
    _numAdditionalPLConstraints += refinement.getPiecewiseLinearConstraints().size();
    // Ownership of the additional constraints are transferred.
    refinement.getPiecewiseLinearConstraints().clear();

    INCREMENTAL_LINEARIZATION_LOG(
        Stringf( "Refined %u non-linear constraints", numRefined ).ascii() );
    return numRefined;
}

void IncrementalLinearization::printStatus()
{
    printf( "\n--- Incremental linearization round %u ---\n", ++_round );
    printf( "Added %u equations, %u piecewise-linear constraints.\n",
            _numAdditionalEquations,
            _numAdditionalPLConstraints );
}

Engine *IncrementalLinearization::releaseEngine()
{
    return _engine.release();
}

void IncrementalLinearization::setInitialTimeoutInMicroSeconds(
    unsigned long long timeoutInMicroSeconds )
{
    _timeoutInMicroSeconds =
        ( timeoutInMicroSeconds == 0 ? FloatUtils::infinity() : timeoutInMicroSeconds );
}

} // namespace CEGAR
