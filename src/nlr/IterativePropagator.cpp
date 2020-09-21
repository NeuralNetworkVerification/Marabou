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

#include "Debug.h"
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
    boost::chrono::milliseconds waitTime( numberOfWorkers - 1 );

    Map<GurobiWrapper *, unsigned> solverToIndex;
    // Create a queue of free workers
    // When a worker is working, it is popped off the queue, when it is done, it
    // is added back to the queue.
    SolverQueue freeSolvers( numberOfWorkers );
    for ( unsigned i = 0; i < numberOfWorkers; ++i )
    {
        GurobiWrapper *gurobi = new GurobiWrapper();
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

    // Variables used for early quitting
    Layer *lastLayer = layers[layers.size() - 1];

    // A NeuronIndex after all real neurons
    NeuronIndex lastIndex( lastLayer->getLayerIndex() + 1, 0 );
    // The neuron fixed from last iteration.
    NeuronIndex lastFixedNeuronFromPreviousIteration = lastIndex;
    // The latest neuron fixed in the current iteration
    NeuronIndex lastFixedNeuronThisIteration = lastIndex;
    bool shouldQuit = false;

    struct timespec gurobiStart;
    (void) gurobiStart;
    struct timespec gurobiEnd;
    (void) gurobiEnd;

    gurobiStart = TimeUtils::sampleMicro();

    do
    {
        if ( Options::get()->getInt( Options::VERBOSITY ) > 0 )
            printf( "Number of tighter bounds found by Gurobi before this iteration: %u. Sign changes: %u. Cutoffs: %u\n",
                    tighterBoundCounter.load(), signChanges.load(), cutoffs.load() );

        mtx.lock();
        lastFixedNeuronFromPreviousIteration = lastFixedNeuronThisIteration;
        lastFixedNeuronThisIteration = lastIndex;
        mtx.unlock();

        DEBUG({
                std::cout << "Last fixed Neuron From Previous Iteration: " <<
                    lastFixedNeuronFromPreviousIteration._layer << " " <<
                    lastFixedNeuronFromPreviousIteration._neuron << std::endl;
            });

        for ( const auto &currentLayer : layers )
        {
            if ( shouldQuit )
                break;
            Layer *layer = currentLayer.second;
            if ( layer->getLayerType() == Layer::INPUT )
                continue;
            for ( unsigned i = 0; i < layer->getSize(); ++i )
            {
                if ( layer->neuronEliminated( i ) )
                    continue;

                currentLb = layer->getLb( i );
                currentUb = layer->getUb( i );

                mtx.lock();
                bool progressMade = lastFixedNeuronThisIteration != lastIndex;
                mtx.unlock();

                if ( !progressMade &&
                     lastFixedNeuronFromPreviousIteration <
                     NeuronIndex( layer->getLayerIndex(), i ) )
                {
                    // If we reached the last fixed neuron from the
                    // previous iteration but this iteration hasn't fixed any neurons
                    if ( Options::get()->getInt( Options::VERBOSITY ) > 0 )
                        printf( "No progress made this iteration, quitting...\n" );

                    for ( unsigned i = 0; i < numberOfWorkers; ++i )
                    {
                        threads[i].interrupt();
                        threads[i].join();
                    }
                    shouldQuit = true;
                    break;
                }

                if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
                    continue;

                if ( Options::get()->getInt( Options::VERBOSITY ) > 1 )
                    printf( "Handling layer %d neuron %d\n",
                            layer->getLayerIndex(), i );

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
                                         std::ref( cutoffs ),
                                         &lastFixedNeuronThisIteration );

                threads[solverToIndex[freeSolver]] = boost::thread
                    ( tightenSingleVariableBounds, argument );
            }
        }

        for ( unsigned i = 0; i < numberOfWorkers; ++i )
        {
            threads[i].join();
        }

        if ( Options::get()->getInt( Options::VERBOSITY ) > 0 )
            printf( "Number of tighter bounds found by Gurobi after this iteration: %u. Sign changes: %u. Cutoffs: %u\n",
                    tighterBoundCounter.load(), signChanges.load(), cutoffs.load() );
    }
    while ( !shouldQuit );

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


double IterativePropagator::optimizeWithGurobi( GurobiWrapper &gurobi, MinOrMax
                                           minOrMax, String variableName,
                                           double cutoffValue,
                                           std::atomic_bool *infeasible )
{
    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    if ( minOrMax == MAX )
        gurobi.setObjective( terms );
    else
        gurobi.setCost( terms );

    gurobi.solve();

    if ( gurobi.infeasbile() )
    {
        if ( infeasible )
        {
            *infeasible = true;
            return FloatUtils::infinity();
        }
        else
            throw InfeasibleQueryException();
    }

    if ( gurobi.cutoffOccurred() )
        return cutoffValue;

    if ( gurobi.optimal() )
    {
        Map<String, double> dontCare;
        double result = 0;
        gurobi.extractSolution( dontCare, result );
        return result;
    }
    else if ( gurobi.timeout() )
    {
        return gurobi.getObjectiveBound();
    }

    throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
}

void IterativePropagator::tightenSingleVariableBounds( ThreadArgument &argument )
{
    try
    {
        // try the phase corresponding to the larger interval first
        if ( -argument._currentLb < argument._currentUb )
        {
            if ( tightenSingleVariableLowerBounds( argument ) )
                tightenSingleVariableUpperBounds( argument );
        }
        else
        {
            if ( tightenSingleVariableUpperBounds( argument ) )
                tightenSingleVariableLowerBounds( argument );
        }
        SolverQueue &freeSolvers = argument._freeSolvers;
        GurobiWrapper *gurobi = argument._gurobi;
        enqueueSolver( freeSolvers, gurobi );
    }
    catch ( boost::thread_interrupted& )
    {
        enqueueSolver( argument._freeSolvers, argument._gurobi );
    }
}

bool IterativePropagator::tightenSingleVariableLowerBounds( ThreadArgument &argument )
{
    GurobiWrapper *gurobi = argument._gurobi;
    Layer *layer = argument._layer;
    unsigned index = argument._index;
    double currentLb = argument._currentLb;
    bool cutoffInUse = argument._cutoffInUse;
    double cutoffValue = argument._cutoffValue;
    LayerOwner *layerOwner = argument._layerOwner;
    std::mutex &mtx = argument._mtx;
    std::atomic_bool &infeasible = argument._infeasible;
    std::atomic_uint &tighterBoundCounter = argument._tighterBoundCounter;
    std::atomic_uint &signChanges = argument._signChanges;
    std::atomic_uint &cutoffs = argument._cutoffs;
    NeuronIndex *lastFixedNeuron = argument._lastFixedNeuron;

    unsigned variable = layer->neuronToVariable( index );
    Stringf variableName( "x%u", variable );
    gurobi->reset();
    double lb = optimizeWithGurobi( *gurobi, MinOrMax::MIN, variableName,
                                    cutoffValue, &infeasible );

    // Store the new bound if it is tighter
    if ( lb > currentLb )
    {
        if ( FloatUtils::isNegative( currentLb ) &&
             !FloatUtils::isNegative( lb ) )
        {
            IterativePropagator_LOG( Stringf( "Neuron(%d, %d) new lower bound non-nagative!",
                                              layer->getLayerIndex(), index
                                              ).ascii() );
            ++signChanges;
            mtx.lock();
            *lastFixedNeuron = NeuronIndex( layer->getLayerIndex(),
                                           index );
            mtx.unlock();
        }

        mtx.lock();
        layer->setLb( index, lb );
        layerOwner->receiveTighterBound( Tightening( variable,
                                                     lb,
                                                     Tightening::LB ) );
        mtx.unlock();
        ++tighterBoundCounter;

        if ( cutoffInUse && lb > cutoffValue )
        {
            ++cutoffs;
            return true;
        }
    }

    return false;
}

bool IterativePropagator::tightenSingleVariableUpperBounds( ThreadArgument &argument )
{
    GurobiWrapper *gurobi = argument._gurobi;
    Layer *layer = argument._layer;
    unsigned index = argument._index;
    double currentUb = argument._currentUb;
    bool cutoffInUse = argument._cutoffInUse;
    double cutoffValue = argument._cutoffValue;
    LayerOwner *layerOwner = argument._layerOwner;
    std::mutex &mtx = argument._mtx;
    std::atomic_bool &infeasible = argument._infeasible;
    std::atomic_uint &tighterBoundCounter = argument._tighterBoundCounter;
    std::atomic_uint &signChanges = argument._signChanges;
    std::atomic_uint &cutoffs = argument._cutoffs;
    NeuronIndex *lastFixedNeuron = argument._lastFixedNeuron;

    unsigned variable = layer->neuronToVariable( index );
    Stringf variableName( "x%u", variable );
    gurobi->reset();
    double ub = optimizeWithGurobi( *gurobi, MinOrMax::MAX, variableName,
                                    cutoffValue, &infeasible );

    // Store the new bound if it is tighter
    if ( ub < currentUb )
    {
        if ( FloatUtils::isPositive( currentUb ) &&
             !FloatUtils::isPositive( ub ) )
        {
            IterativePropagator_LOG( Stringf( "Neuron(%d, %d) new  upper bound non-positive!",
                                              layer->getLayerIndex(), index
                                              ).ascii() );
            ++signChanges;
            mtx.lock();
            *lastFixedNeuron = NeuronIndex( layer->getLayerIndex(),
                                            index );
            mtx.unlock();
        }

        mtx.lock();
        layer->setUb( index, ub );
        layerOwner->receiveTighterBound( Tightening( variable,
                                                     ub,
                                                     Tightening::UB ) );
        mtx.unlock();

        ++tighterBoundCounter;

        if ( cutoffInUse && ub < cutoffValue )
        {
            ++cutoffs;
            return true;
        }
    }

    return false;
}

} // namespace NLR
