/*********************                                                        */
/*! \file NetworkLevelReasoner.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "GurobiWrapper.h"
#include "InfeasibleQueryException.h"
#include "LPFormulator.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include "Options.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

LPFormulator::LPFormulator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _cutoffInUse( false )
    , _cutoffValue( 0 )
{
}

LPFormulator::~LPFormulator()
{
}

double LPFormulator::solveLPRelaxation( GurobiWrapper &gurobi,
                                        const Map<unsigned, Layer *> &layers,
                                        MinOrMax minOrMax, String variableName,
                                        unsigned lastLayer )
{
    gurobi.resetModel();
    createLPRelaxation( layers, gurobi, lastLayer );
    return optimizeWithGurobi( gurobi, minOrMax, variableName, _cutoffValue );
}

double LPFormulator::optimizeWithGurobi( GurobiWrapper &gurobi,
                                         MinOrMax minOrMax, String variableName,
                                         double cutoffValue, std::atomic_bool *infeasible )
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

void LPFormulator::optimizeBoundsWithIncrementalLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    GurobiWrapper gurobi;

    List<GurobiWrapper::Term> terms;
    Map<String, double> dontCare;
    double lb = 0;
    double ub = 0;
    double currentLb = 0;
    double currentUb = 0;

    unsigned tighterBoundCounter = 0;
    unsigned signChanges = 0;
    unsigned cutoffs = 0;

    struct timespec gurobiStart;
    (void) gurobiStart;
    struct timespec gurobiEnd;
    (void) gurobiEnd;

    gurobiStart = TimeUtils::sampleMicro();

    for ( unsigned i = 0; i < _layerOwner->getNumberOfLayers(); ++i )
    {
        /*
          Go over the layers, one by one. Each time encode the layer,
          and then issue queries on each of its variables
        */
        ASSERT( layers.exists( i ) );
        Layer *layer = layers[i];
        addLayerToModel( gurobi, layer );

        for ( unsigned j = 0; j < layer->getSize(); ++j )
        {
            if ( layer->neuronEliminated( j ) )
                continue;

            currentLb = layer->getLb( j );
            currentUb = layer->getUb( j );

            if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
                continue;

            unsigned variable = layer->neuronToVariable( j );
            Stringf variableName( "x%u", variable );

            terms.clear();
            terms.append( GurobiWrapper::Term( 1, variableName ) );

            // Maximize
            gurobi.reset();
            gurobi.setObjective( terms );
            gurobi.solve();

            if ( gurobi.infeasbile() )
                throw InfeasibleQueryException();

            if ( gurobi.cutoffOccurred() )
            {
                ub = _cutoffValue;
            }
            else if ( gurobi.optimal() )
            {
                gurobi.extractSolution( dontCare, ub );
            }
            else if ( gurobi.timeout() )
            {
                ub = gurobi.getObjectiveBound();
            }
            else
            {
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
            }

            // If the bound is tighter, store it
            if ( ub < currentUb )
            {
                gurobi.setUpperBound( variableName, ub );

                if ( FloatUtils::isPositive( currentUb ) &&
                     !FloatUtils::isPositive( ub ) )
                    ++signChanges;

                layer->setUb( j, ub );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              ub,
                                                              Tightening::UB ) );
                ++tighterBoundCounter;

                if ( _cutoffInUse && ub < _cutoffValue )
                {
                    ++cutoffs;
                    continue;
                }
            }

            // Minimize
            gurobi.reset();
            gurobi.setCost( terms );
            gurobi.solve();

            if ( gurobi.infeasbile() )
                throw InfeasibleQueryException();

            if ( gurobi.cutoffOccurred() )
            {
                lb = _cutoffValue;
            }
            else if ( gurobi.optimal() )
            {
                gurobi.extractSolution( dontCare, lb );
            }
            else if ( gurobi.timeout() )
            {
                lb = gurobi.getObjectiveBound();
            }
            else
            {
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
            }

            // If the bound is tighter, store it
            if ( lb > currentLb )
            {
                gurobi.setLowerBound( variableName, lb );

                if ( FloatUtils::isNegative( currentLb ) &&
                     !FloatUtils::isNegative( lb ) )
                    ++signChanges;

                layer->setLb( j, lb );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              lb,
                                                              Tightening::LB ) );
                ++tighterBoundCounter;

                if ( _cutoffInUse && lb > _cutoffValue )
                {
                    ++cutoffs;
                    continue;
                }
            }
        }
    }

    gurobiEnd = TimeUtils::sampleMicro();

    LPFormulator_LOG( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                               tighterBoundCounter, signChanges, cutoffs ).ascii() );
    LPFormulator_LOG( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ).ascii() );
}

void LPFormulator::optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers )
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

    boost::thread *threads = new boost::thread[numberOfWorkers];
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

    bool skipTightenLb = false; // If true, skip lower bound tightening
    bool skipTightenUb = false; // If true, skip upper bound tightening

    unsigned skipTighterLowerBoundCounter = 0;
    unsigned skipTighterUpperBoundCounter = 0;

    for ( const auto &currentLayer : layers )
    {
        Layer *layer = currentLayer.second;

        // declare simulations as local var to avoid a problem which can happen due to multi thread process.
        const std::vector<std::vector<double>> *simulations = _layerOwner->getLayer( currentLayer.first )->getSimulations();

        for ( unsigned i = 0; i < layer->getSize(); ++i )
        {
            if ( layer->neuronEliminated( i ) )
                continue;

            currentLb = layer->getLb( i );
            currentUb = layer->getUb( i );

            if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
                continue;

            skipTightenLb = false;
            skipTightenUb = false;

            // Loop for simulation
            // for ( const auto &simValue : ( *( _layerOwner->getLayer( currentLayer.first )->getSimulations() ) )[i] )
            for ( const auto &simValue : (*simulations)[i] )
            {
                if ( _cutoffInUse && _cutoffValue < simValue ) // If x_lower < 0 < x_sim, do not try to call tightning upper bound.
                    skipTightenUb = true;

                if ( _cutoffInUse && simValue <= _cutoffValue ) // If x_sim < 0 < x_upper, do not try to call tightning lower bound.
                    skipTightenLb = true;

                if ( skipTightenUb && skipTightenLb )
                    break;
            }

            // If no tightning is needed, continue
            if ( skipTightenUb && skipTightenLb )
            {
                mtx.lock();
                ++skipTighterUpperBoundCounter;
                ++skipTighterLowerBoundCounter;
                mtx.unlock();
                continue;
            }
            else if ( skipTightenUb )
            {
                mtx.lock();
                ++skipTighterUpperBoundCounter;
                mtx.unlock();
            }
            else if ( skipTightenLb )
            {
                mtx.lock();
                ++skipTighterLowerBoundCounter;
                mtx.unlock();
            }

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
            createLPRelaxation( layers, *freeSolver, layer->getLayerIndex() );
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
                                     skipTightenLb, 
                                     skipTightenUb );

            if ( numberOfWorkers == 1 )
                tightenSingleVariableBoundsWithLPRelaxation( argument );
            else
                threads[solverToIndex[freeSolver]] = boost::thread
                    ( tightenSingleVariableBoundsWithLPRelaxation, argument );
        }
    }

    for ( unsigned i = 0; i < numberOfWorkers; ++i )
    {
        threads[i].join();
    }

    gurobiEnd = TimeUtils::sampleMicro();

    LPFormulator_LOG( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                               tighterBoundCounter.load(), signChanges.load(), cutoffs.load() ).ascii() );
    LPFormulator_LOG( Stringf( "Number of skipped tighter lower bounds by simulation: %u\n", skipTighterLowerBoundCounter ) );
    LPFormulator_LOG( Stringf( "Number of skipped tighter upper bounds by simulation: %u\n", skipTighterUpperBoundCounter ) );
    LPFormulator_LOG( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ).ascii() );

    clearSolverQueue( freeSolvers );

    if ( infeasible )
        throw InfeasibleQueryException();
}

void LPFormulator::tightenSingleVariableBoundsWithLPRelaxation( ThreadArgument &argument )
{
    try
    {
        GurobiWrapper *gurobi = argument._gurobi;
        Layer *layer = argument._layer;
        unsigned index = argument._index;
        double currentLb = argument._currentLb;
        double currentUb = argument._currentUb;
        bool cutoffInUse = argument._cutoffInUse;
        double cutoffValue = argument._cutoffValue;
        LayerOwner *layerOwner = argument._layerOwner;
        SolverQueue &freeSolvers = argument._freeSolvers;
        std::mutex &mtx = argument._mtx;
        std::atomic_bool &infeasible = argument._infeasible;
        std::atomic_uint &tighterBoundCounter = argument._tighterBoundCounter;
        std::atomic_uint &signChanges = argument._signChanges;
        std::atomic_uint &cutoffs = argument._cutoffs;
        bool skipTightenLb = argument._skipTightenLb;
        bool skipTightenUb = argument._skipTightenUb;

        LPFormulator_LOG( Stringf( "Tightening bounds for layer %u index %u",
                                   layer->getLayerIndex(), index ).ascii() );

        unsigned variable = layer->neuronToVariable( index );
        Stringf variableName( "x%u", variable );

        if ( !skipTightenUb )
        {
            LPFormulator_LOG( Stringf( "Computing upperbound..." ).ascii() );
            double ub = optimizeWithGurobi( *gurobi, MinOrMax::MAX, variableName,
                                            cutoffValue, &infeasible );
            LPFormulator_LOG( Stringf( "Upperbound computed %f", ub ).ascii() );

            // Store the new bound if it is tighter
            if ( ub < currentUb )
            {
                if ( FloatUtils::isPositive( currentUb ) &&
                    !FloatUtils::isPositive( ub ) )
                    ++signChanges;

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
                    enqueueSolver( freeSolvers, gurobi );
                    return;
                }
            }
        }

        if ( !skipTightenLb )
        {
            LPFormulator_LOG( Stringf( "Computing lowerbound..." ).ascii() );
            gurobi->reset();
            double lb = optimizeWithGurobi( *gurobi, MinOrMax::MIN, variableName,
                                            cutoffValue, &infeasible );
            LPFormulator_LOG( Stringf( "Lowerbound computed: %f", lb ).ascii() );
            // Store the new bound if it is tighter
            if ( lb > currentLb )
            {
                if ( FloatUtils::isNegative( currentLb ) &&
                    !FloatUtils::isNegative( lb ) )
                    ++signChanges;

                mtx.lock();
                layer->setLb( index, lb );
                layerOwner->receiveTighterBound( Tightening( variable,
                                                            lb,
                                                            Tightening::LB ) );
                mtx.unlock();
                ++tighterBoundCounter;

                if ( cutoffInUse && lb > cutoffValue )
                    ++cutoffs;
            }
        }
        enqueueSolver( freeSolvers, gurobi );
    }
    catch ( boost::thread_interrupted& )
    {
        enqueueSolver( argument._freeSolvers, argument._gurobi );
    }
}

void LPFormulator::createLPRelaxation( const Map<unsigned, Layer *> &layers,
                                       GurobiWrapper &gurobi,
                                       unsigned lastLayer )
{
    for ( const auto &layer : layers )
    {
        if ( layer.second->getLayerIndex() > lastLayer )
            continue;

        addLayerToModel( gurobi, layer.second );
    }
}

void LPFormulator::addLayerToModel( GurobiWrapper &gurobi, const Layer *layer )
{
    switch ( layer->getLayerType() )
    {
    case Layer::INPUT:
        addInputLayerToLpRelaxation( gurobi, layer );
        break;

    case Layer::RELU:
        addReluLayerToLpRelaxation( gurobi, layer );
        break;

    case Layer::WEIGHTED_SUM:
        addWeightedSumLayerToLpRelaxation( gurobi, layer );
        break;

    case Layer::SIGN:
        addSignLayerToLpRelaxation( gurobi, layer );
        break;

    case Layer::MAX:
        addMaxLayerToLpRelaxation( gurobi, layer );
        break;

    default:
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED, "LPFormulator" );
        break;
    }
}

void LPFormulator::addInputLayerToLpRelaxation( GurobiWrapper &gurobi,
                                                const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        unsigned variable = layer->neuronToVariable( i );
        gurobi.addVariable( Stringf( "x%u", variable ),
                            layer->getLb( i ),
                            layer->getUb( i ) );
    }
}

void LPFormulator::addReluLayerToLpRelaxation( GurobiWrapper &gurobi,
                                               const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        if ( !layer->neuronEliminated( i ) )
        {
            unsigned targetVariable = layer->neuronToVariable( i );

            List<NeuronIndex> sources = layer->getActivationSources( i );
            const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );
            unsigned sourceNeuron = sources.begin()->_neuron;

            if ( sourceLayer->neuronEliminated( sourceNeuron ) )
            {
                // If the source neuron has been eliminated, this neuron is constant
                double sourceValue = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
                double targetValue = sourceValue > 0 ? sourceValue : 0;

                gurobi.addVariable( Stringf( "x%u", targetVariable ),
                                    targetValue,
                                    targetValue );

                continue;
            }

            unsigned sourceVariable = sourceLayer->neuronToVariable( sourceNeuron );
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            gurobi.addVariable( Stringf( "x%u", targetVariable ),
                                0,
                                layer->getUb( i ) );

            if ( !FloatUtils::isNegative( sourceLb ) )
            {
                // The ReLU is active, y = x
                if ( sourceLb < 0 )
                    sourceLb = 0;

                List<GurobiWrapper::Term> terms;
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
                gurobi.addEqConstraint( terms, 0 );
            }
            else if ( !FloatUtils::isPositive( sourceUb ) )
            {
                // The ReLU is inactive, y = 0
                List<GurobiWrapper::Term> terms;
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                gurobi.addEqConstraint( terms, 0 );
            }
            else
            {
                /*
                  The phase of this ReLU is not yet fixed.

                  For y = ReLU(x), we add the following triangular relaxation:

                  1. y >= 0
                  2. y >= x
                  3. y is below the line the crosses (x.lb,0) and (x.ub,x.ub)
                */

                // y >= 0
                List<GurobiWrapper::Term> terms;
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                gurobi.addGeqConstraint( terms, 0 );

                // y >= x, i.e. y - x >= 0
                terms.clear();
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
                gurobi.addGeqConstraint( terms, 0 );

                /*
                         u        ul
                  y <= ----- x - -----
                       u - l     u - l
                */
                terms.clear();
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                terms.append( GurobiWrapper::Term( -sourceUb / ( sourceUb - sourceLb ), Stringf( "x%u", sourceVariable ) ) );
                gurobi.addLeqConstraint( terms, ( -sourceUb * sourceLb ) / ( sourceUb - sourceLb ) );
            }
        }
    }
}

void LPFormulator::addSignLayerToLpRelaxation( GurobiWrapper &gurobi,
                                               const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        if ( layer->neuronEliminated( i ) )
            continue;

        unsigned targetVariable = layer->neuronToVariable( i );

        List<NeuronIndex> sources = layer->getActivationSources( i );
        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );
        unsigned sourceNeuron = sources.begin()->_neuron;

        if ( sourceLayer->neuronEliminated( sourceNeuron ) )
        {
            // If the source neuron has been eliminated, this neuron is constant
            double sourceValue = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
            double targetValue = FloatUtils::isNegative( sourceValue ) ? -1 : 1;

            gurobi.addVariable( Stringf( "x%u", targetVariable ),
                                targetValue,
                                targetValue );

            continue;
        }

        unsigned sourceVariable = sourceLayer->neuronToVariable( sourceNeuron );
        double sourceLb = sourceLayer->getLb( sourceNeuron );
        double sourceUb = sourceLayer->getUb( sourceNeuron );

        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            // The Sign is positive, y = 1
            gurobi.addVariable( Stringf( "x%u", targetVariable ), 1, 1 );
        }
        else if ( FloatUtils::isNegative( sourceUb ) )
        {
            // The Sign is negative, y = -1
            gurobi.addVariable( Stringf( "x%u", targetVariable ), -1, -1 );
        }
        else
        {
            /*
              The phase of this Sign is not yet fixed.

              For y = Sign(x), we add the following parallelogram relaxation:

              1. y >= -1
              2. y <= -1
              3. y is below the line the crosses (x.lb,-1) and (0,1)
              4. y is above the line the crosses (0,-1) and (x.ub,1)
            */

            // -1 <= y <= 1
            gurobi.addVariable( Stringf( "x%u", targetVariable ), -1, 1 );

            /*
                     2
              y <= ----- x + 1
                    - l
            */
            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
            terms.append( GurobiWrapper::Term( 2.0 / sourceLb, Stringf( "x%u", sourceVariable ) ) );
            gurobi.addLeqConstraint( terms, 1 );

            /*
                     2
              y >= ----- x - 1
                     u
            */
            terms.clear();
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
            terms.append( GurobiWrapper::Term( -2.0 / sourceUb, Stringf( "x%u", sourceVariable ) ) );
            gurobi.addGeqConstraint( terms, -1 );
        }
    }
}

void LPFormulator::addMaxLayerToLpRelaxation( GurobiWrapper &gurobi,
                                              const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        if ( layer->neuronEliminated( i ) )
            continue;

        unsigned targetVariable = layer->neuronToVariable( i );
        gurobi.addVariable( Stringf( "x%u", targetVariable ), layer->getLb( i ), layer->getUb( i ) );

        List<NeuronIndex> sources = layer->getActivationSources( i );

        bool haveFixedSourceValue = false;
        double maxFixedSourceValue = FloatUtils::negativeInfinity();

        double maxConcreteUb = FloatUtils::negativeInfinity();

        List<GurobiWrapper::Term> terms;

        for ( const auto &source : sources )
        {
            const Layer *sourceLayer = _layerOwner->getLayer( source._layer );
            unsigned sourceNeuron = source._neuron;

            if ( sourceLayer->neuronEliminated( sourceNeuron ) )
            {
                haveFixedSourceValue = true;
                double value = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
                if ( value > maxFixedSourceValue )
                    maxFixedSourceValue = value;
                continue;
            }

            unsigned sourceVariable = sourceLayer->neuronToVariable( sourceNeuron );

            // Target is at least source: target - source >= 0
            terms.clear();
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
            terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
            gurobi.addGeqConstraint( terms, 0 );

            // Find maximal concrete upper bound
            double sourceUb = sourceLayer->getUb( sourceNeuron );
            if ( sourceUb > maxConcreteUb )
                maxConcreteUb = sourceUb;
        }

        if ( haveFixedSourceValue && ( maxConcreteUb < maxFixedSourceValue ) )
        {
            // At least one of the sources has a fixed value,
            // and this fixed value dominates other sources.
            terms.clear();
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
            gurobi.addEqConstraint( terms, maxFixedSourceValue );
        }
        else
        {
            // If we have a fixed value, it's a lower bound
            if ( haveFixedSourceValue )
            {
                terms.clear();
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                gurobi.addGeqConstraint( terms, maxFixedSourceValue );
            }

            // Target must be smaller than greatest concrete upper bound
            terms.clear();
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
            gurobi.addLeqConstraint( terms, maxConcreteUb );
        }
    }
}

void LPFormulator::addWeightedSumLayerToLpRelaxation( GurobiWrapper &gurobi, const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        if ( !layer->neuronEliminated( i ) )
        {
            unsigned variable = layer->neuronToVariable( i );

            gurobi.addVariable( Stringf( "x%u", variable ),
                                layer->getLb( i ),
                                layer->getUb( i ) );

            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", variable ) ) );

            double bias = -layer->getBias( i );

            for ( const auto &sourceLayerPair : layer->getSourceLayers() )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerPair.first );
                unsigned sourceLayerSize = sourceLayerPair.second;

                for ( unsigned j = 0; j < sourceLayerSize; ++j )
                {
                    double weight = layer->getWeight( sourceLayerPair.first, j, i );
                    if ( !sourceLayer->neuronEliminated( j ) )
                    {
                        Stringf sourceVariableName( "x%u",
                                                    sourceLayer->neuronToVariable( j ) );
                        terms.append( GurobiWrapper::Term( weight, sourceVariableName ) );
                    }
                    else
                    {
                        bias -= weight * sourceLayer->getEliminatedNeuronValue( j );
                    }
                }
            }

            gurobi.addEqConstraint( terms, bias );
        }
    }
}

void LPFormulator::setCutoff( double cutoff )
{
    _cutoffInUse = true;
    _cutoffValue = cutoff;
}

} // namespace NLR
