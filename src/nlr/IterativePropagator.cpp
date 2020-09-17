/*********************                                                        */
/*! \file IterativePropagator.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "InfeasibleQueryException.h"
#include "IterativePropagator.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include "Options.h"
#include "TimeUtils.h"

#include "Vector.h"

#include <boost/thread.hpp>

namespace NLR {

IterativePropagator::IterativePropagator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _milpFormulator( layerOwner )
    , _cutoffInUse( false )
    , _cutoffValue( 0 )
{
}

IterativePropagator::~IterativePropagator()
{
}

void IterativePropagator::optimizeBoundsWithIterativePropagation( const Map<unsigned, Layer *> &layers )
{
    unsigned numberOfWorkers = Options::get()->getInt( Options::NUM_WORKERS );

    // Time to wait if no idle worker is availble
    boost::chrono::milliseconds waitTime ( numberOfWorkers - 1 );

    double perReLUTimeout = Options::get()->getFloat
        ( Options::MILP_SOLVER_TIMEOUT );

    Map<GurobiWrapper *, unsigned> solverToIndex;
    // Create a queue of free workers
    // When a worker is working, it is popped off the queue, when it is done, it
    // is added back to the queue.
    SolverQueue freeSolvers ( numberOfWorkers );
    for ( unsigned i = 0; i < numberOfWorkers; ++i )
    {
        GurobiWrapper *gurobi = new GurobiWrapper();
        gurobi->setTimeLimit( perReLUTimeout );
        solverToIndex[gurobi] = i;
        enqueueSolver( freeSolvers, gurobi );
    }

    std::vector<boost::thread> threads( numberOfWorkers );
    std::mutex mtx;
    std::atomic_bool infeasible( false );

    double currentLb;
    double currentUb;

    std::atomic_uint tighterBoundCounter( 0 );
    std::atomic_uint signChanges( 0 );
    std::atomic_uint cutoffs( 0 );

    struct timespec gurobiStart;
    (void) gurobiStart;
    struct timespec gurobiEnd;
    (void) gurobiEnd;

    gurobiStart = TimeUtils::sampleMicro();

    for ( const auto &currentLayer : layers )
    {
        Layer *layer = currentLayer.second;
        if ( layer->getLayerType() == Layer::INPUT )
            continue;
        for ( unsigned i = 0; i < layer->getSize(); ++i )
        {
            if ( layer->neuronEliminated( i ) )
                continue;

            currentLb = layer->getLb( i );
            currentUb = layer->getUb( i );

            if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
                continue;

            if ( infeasible )
            {
                // infeasibility is derived, interupt all active threads
                for ( unsigned i = 0; i < numberOfWorkers; ++i )
                {
                    threads[i].interrupt();
                    threads[i].join();
                }
                clearSolverQueue( freeSolvers );
                throw InfeasibleQueryException();
            }

            // Wait until there is an idle solver
            GurobiWrapper *freeSolver;
            while ( !freeSolvers.pop( freeSolver ) )
                boost::this_thread::sleep_for( waitTime );

            freeSolver->resetModel();
            freeSolver->setTimeLimit( perReLUTimeout );
            mtx.lock();
            _milpFormulator.createMILPEncoding
                ( layers, *freeSolver, _layerOwner->getNumberOfLayers() );
            mtx.unlock();

            // spawn a thread to tighten the bounds for the current variable
            ThreadArgument argument( freeSolver, layer,
                                     i, currentLb, currentUb,
                                     _cutoffInUse, _cutoffValue,
                                     _layerOwner, std::ref( freeSolvers ),
                                     std::ref( mtx ), std::ref( infeasible ),
                                     std::ref( tighterBoundCounter ),
                                     std::ref( signChanges ),
                                     std::ref( cutoffs ) );

            threads[solverToIndex[freeSolver]] = boost::thread
                ( tightenSingleVariableBounds, argument );
        }
    }

    for ( unsigned i = 0; i < numberOfWorkers; ++i )
    {
        threads[i].join();
    }

    gurobiEnd = TimeUtils::sampleMicro();

    IterativePropagator_LOG( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                               tighterBoundCounter.load(), signChanges.load(), cutoffs.load() ).ascii() );
    IterativePropagator_LOG( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ).ascii() );

    clearSolverQueue( freeSolvers );

    if ( infeasible )
        throw InfeasibleQueryException();
}

void IterativePropagator::setCutoff( double cutoff )
{
    _cutoffInUse = true;
    _cutoffValue = cutoff;
}

} // namespace NLR
