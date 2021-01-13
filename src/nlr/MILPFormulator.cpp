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
#include "MILPFormulator.h"
#include "MStringf.h"
#include "NLRError.h"
#include "Options.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

MILPFormulator::MILPFormulator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _lpFormulator( layerOwner )
    , _signChanges( 0 )
    , _tighterBoundCounter( 0 )
    , _cutoffs( 0 )
    , _cutoffInUse( false )
    , _cutoffValue( 0 )
{
}

MILPFormulator::~MILPFormulator()
{
}

void MILPFormulator::optimizeBoundsWithIncrementalMILPEncoding( const Map<unsigned, Layer *> &layers )
{
    _tighterBoundCounter = 0;
    _signChanges = 0;
    _cutoffs = 0;

    GurobiWrapper gurobi;

    double currentLb;
    double currentUb;
    List<GurobiWrapper::Term> terms;
    Map<String, double> dontCare;

    struct timespec gurobiStart = TimeUtils::sampleMicro();

    for ( unsigned i = 0; i < _layerOwner->getNumberOfLayers(); ++i )
    {
        /*
          Go over the layers, one by one. Each time encode the layer,
          and then issue queries on each of its variables
        */
        ASSERT( layers.exists( i ) );
        Layer *layer = layers[i];
        _lpFormulator.addLayerToModel( gurobi, layer );

        /*
          The optimiziation is performed layer by layer, and for each
          individual neuron. It has 4 steps:

          1. Use an LP relaxation to maximize the variable
          2. Use an LP relaxation to minimize the variable
          3. Use a MILP encoding to maximize the variable
          4. Use a MILP encoding to minimize the variable

          We perform the steps in this order, and stop if at some
          point we discover a bound that crosses the cutoff value, we
          stop.
        */
        bool layerRequiresMILP = layerRequiresMILPEncoding( layer );
        for ( unsigned j = 0; j < layer->getSize(); ++j )
        {
            if ( layer->neuronEliminated( j ) )
                continue;

            currentLb = layer->getLb( j );
            currentUb = layer->getUb( j );

            if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
            {
                if ( layerRequiresMILP )
                    addNeuronToModel( gurobi, layer, j, _layerOwner );
                continue;
            }

            unsigned variable = layer->neuronToVariable( j );
            Stringf variableName( "x%u", variable );

            terms.clear();
            terms.append( GurobiWrapper::Term( 1, variableName ) );

            // Maximize, using just the LP relaxation for the current layer
            if ( tightenUpperBound( gurobi, layer, j, variable, currentUb ) )
            {
                if ( layerRequiresMILP )
                    addNeuronToModel( gurobi, layer, j, _layerOwner );
                continue;
            }

            // Minimize, using just the LP relaxation for the current layer
            if ( tightenLowerBound( gurobi, layer, j, variable, currentLb ) )
            {
                if ( layerRequiresMILP )
                    addNeuronToModel( gurobi, layer, j, _layerOwner );
                continue;
            }

            // If the current layer is a weighted sum layer, the MILP
            // encoding doesn't add any additional assertions to the
            // model, and we can stop here.
            if ( !layerRequiresMILP )
                continue;

            addNeuronToModel( gurobi, layer, j, _layerOwner );

            // Maximize, using just the exact MILP encoding
            if ( tightenUpperBound( gurobi, layer, j, variable, currentUb ) )
                continue;

            // Minimize, using just the exact MILP encoding
            if ( tightenLowerBound( gurobi, layer, j, variable, currentLb ) )
                continue;
        }
    }

    struct timespec gurobiEnd = TimeUtils::sampleMicro();

    log( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                  _tighterBoundCounter, _signChanges, _cutoffs ) );
    log( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ) );
}

void MILPFormulator::optimizeBoundsWithMILPEncoding( const Map<unsigned, Layer *> &layers )
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

    struct timespec gurobiStart = TimeUtils::sampleMicro();

    for ( const auto &currentLayer : layers )
    {
        Layer *layer = currentLayer.second;

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

            mtx.lock();
            _lpFormulator.createLPRelaxation( layers, *freeSolver, layer->getLayerIndex() );
            mtx.unlock();

            // spawn a thread to tighten the bounds for the current variable
            ThreadArgument argument( freeSolver, layer, &layers,
                                     i, currentLb, currentUb,
                                     _cutoffInUse, _cutoffValue,
                                     _layerOwner, std::ref( freeSolvers ),
                                     std::ref( mtx ), std::ref( infeasible ),
                                     std::ref( tighterBoundCounter ),
                                     std::ref( signChanges ),
                                     std::ref( cutoffs ) );

            if ( numberOfWorkers == 1 )
                tightenSingleVariableBoundsWithMILPEncoding( argument );
            else
                threads[solverToIndex[freeSolver]] = boost::thread
                    ( tightenSingleVariableBoundsWithMILPEncoding, argument );
        }
    }

    for ( unsigned i = 0; i < numberOfWorkers; ++i )
    {
        threads[i].join();
    }

    struct timespec gurobiEnd = TimeUtils::sampleMicro();

    log( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                  tighterBoundCounter.load(), signChanges.load(), cutoffs.load() ) );
    log( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ) );

    clearSolverQueue( freeSolvers );

    if ( infeasible )
        throw InfeasibleQueryException();
}

void MILPFormulator::tightenSingleVariableBoundsWithMILPEncoding( ThreadArgument &argument )
{
    try
    {
        /*
          The optimiziation is performed layer by layer, and for each
          individual neuron. It has 4 steps:

          1. Use an LP relaxation to minimize the variable
          2. Use an LP relaxation to maximize the variable
          3. Use a MILP encoding to minimize the variable
          4. Use a MILP encoding to maximize the variable

          We perform the steps in this order, and stop if at some
          point we discover either an upper bound that is non-positive
          or a lower obund that is non-negative (this is aimed at
          ReLUs, as their phase would become fixed in these cases)
        */

        GurobiWrapper *gurobi = argument._gurobi;
        Layer *layer = argument._layer;
        const Map<unsigned, Layer *> &layers = *( argument._layers );
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

        // LP Relaxation
        log( Stringf( "Tightening bounds for layer %u index %u",
                                   layer->getLayerIndex(), index ).ascii() );

        unsigned variable = layer->neuronToVariable( index );
        Stringf variableName( "x%u", variable );

        log( Stringf( "Computing lowerbound..." ).ascii() );
        double lb = optimizeWithGurobi( *gurobi, MinOrMax::MIN, variableName,
                                        cutoffValue, &infeasible );
        log( Stringf( "Lowerbound computed: %f", lb ).ascii() );

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
            {
                ++cutoffs;
                enqueueSolver( freeSolvers, gurobi );
                return;
            }
        }

        log( Stringf( "Computing upperbound..." ).ascii() );
        gurobi->reset();
        double ub = optimizeWithGurobi( *gurobi, MinOrMax::MAX, variableName,
                                        cutoffValue, &infeasible );
        log( Stringf( "Upperbound computed %f", ub ).ascii() );

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

        gurobi->reset();
        // Exact encoding
        // Now, add the MILP constraints
        unsigned lastLayer = layer->getLayerIndex();
        for ( const auto &layer : layers )
        {
            if ( layer.second->getLayerIndex() > lastLayer )
                continue;

            addLayerToModel( *gurobi, layer.second, layerOwner );
        }

        log( Stringf( "Computing lowerbound..." ).ascii() );
        lb = optimizeWithGurobi( *gurobi, MinOrMax::MIN, variableName,
                                 cutoffValue, &infeasible );
        log( Stringf( "Lowerbound computed: %f", lb ).ascii() );

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
            {
                ++cutoffs;
                enqueueSolver( freeSolvers, gurobi );
                return;
            }
        }

        log( Stringf( "Tightening bounds for layer %u index %u",
                                   layer->getLayerIndex(), index ).ascii() );

        log( Stringf( "Computing upperbound..." ).ascii() );
        gurobi->reset();
        ub = optimizeWithGurobi( *gurobi, MinOrMax::MAX, variableName,
                                 cutoffValue, &infeasible );
        log( Stringf( "Upperbound computed %f", ub ).ascii() );

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
            }
        }

        enqueueSolver( freeSolvers, gurobi );
    }
    catch ( boost::thread_interrupted& )
    {
        enqueueSolver( argument._freeSolvers, argument._gurobi );
    }
}

void MILPFormulator::createMILPEncoding( const Map<unsigned, Layer *> &layers,
                                         GurobiWrapper &gurobi,
                                         unsigned lastLayer )
{
    // First, create the LP relaxation of the problem
    _lpFormulator.createLPRelaxation( layers, gurobi, lastLayer );

    // Now, add the MILP constraints
    for ( const auto &layer : layers )
    {
        if ( layer.second->getLayerIndex() > lastLayer )
            continue;

        addLayerToModel( gurobi, layer.second, _layerOwner );
    }
}

void MILPFormulator::addLayerToModel( GurobiWrapper &gurobi, const Layer *layer,
                                      LayerOwner *layerOwner )
{
    switch ( layer->getLayerType() )
    {
        case Layer::INPUT:
        case Layer::WEIGHTED_SUM:
            break;

        case Layer::RELU:
            addReluLayerToMILPFormulation( gurobi, layer, layerOwner );
            break;

        default:
            throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED, "MILPFormulator" );
            break;
    }
}

void MILPFormulator::addNeuronToModel( GurobiWrapper &gurobi, const Layer *layer,
                                       unsigned neuron, LayerOwner *layerOwner )
{
    if ( layer->getLayerType() != Layer::RELU )
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED, "MILPFormulator" );

    if ( layer->neuronEliminated( neuron ) )
        return;

    unsigned targetVariable = layer->neuronToVariable( neuron );

    List<NeuronIndex> sources = layer->getActivationSources( neuron );
    const Layer *sourceLayer = layerOwner->getLayer( sources.begin()->_layer );
    unsigned sourceNeuron = sources.begin()->_neuron;
    unsigned sourceVariable = sourceLayer->neuronToVariable( sourceNeuron );

    double sourceLb = sourceLayer->getLb( sourceNeuron );
    double sourceUb = sourceLayer->getUb( sourceNeuron );

    if ( !FloatUtils::isNegative( sourceLb ) ||
         !FloatUtils::isPositive( sourceUb ) )
    {
        // This ReLU is fixed in one of its phases
        return;
    }

    /*
      The underlying LP relaxation defines the triangular
      region; we add the indicator variable a \in {0,1}:

      y <= x - l ( 1 - a )
      y <= u a

      Or, alternatively:

      y - x -la <= -l
      y - ua <= 0
    */

    gurobi.addVariable( Stringf( "a%u", targetVariable ),
                        0,
                        1,
                        GurobiWrapper::BINARY );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceLb, Stringf( "a%u", targetVariable ) ) );
    gurobi.addLeqConstraint( terms, -sourceLb );

    terms.clear();
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceUb, Stringf( "a%u", targetVariable ) ) );
    gurobi.addLeqConstraint( terms, 0 );
}

void MILPFormulator::addReluLayerToMILPFormulation( GurobiWrapper &gurobi,
                                                    const Layer *layer,
                                                    LayerOwner *layerOwner )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        addNeuronToModel( gurobi, layer, i, layerOwner );
    }
}

double MILPFormulator::optimizeWithGurobi( GurobiWrapper &gurobi,
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

void MILPFormulator::storeUbIfNeeded( Layer *layer, unsigned neuron, unsigned variable, double newUb )
{
    double ub = layer->getUb( neuron );
    if ( newUb < ub )
    {
        if ( FloatUtils::isPositive( ub ) && !FloatUtils::isPositive( newUb ) )
            ++_signChanges;

        layer->setUb( neuron, newUb );
        _layerOwner->receiveTighterBound( Tightening( variable,
                                                      newUb,
                                                      Tightening::UB ) );
        ++_tighterBoundCounter;
    }
}

void MILPFormulator::storeLbIfNeeded( Layer *layer, unsigned neuron, unsigned variable, double newLb )
{
    double lb = layer->getLb( neuron );
    if ( newLb > lb )
    {
        if ( FloatUtils::isNegative( lb ) && !FloatUtils::isNegative( newLb ) )
            ++_signChanges;

        layer->setLb( neuron, newLb );
        _layerOwner->receiveTighterBound( Tightening( variable,
                                                      newLb,
                                                      Tightening::LB ) );
        ++_tighterBoundCounter;
    }
}

void MILPFormulator::setCutoff( double cutoff )
{
    _cutoffInUse = true;
    _cutoffValue = cutoff;
}

bool MILPFormulator::tightenUpperBound( GurobiWrapper &gurobi,
                                        Layer *layer,
                                        unsigned neuron,
                                        unsigned variable,
                                        double &currentUb )
{
    double newUb = FloatUtils::infinity();

    Stringf variableName( "x%u", variable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    gurobi.reset();
    gurobi.setObjective( terms );
    gurobi.solve();

    if ( gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( gurobi.cutoffOccurred() )
    {
        newUb = _cutoffValue;
    }
    else if ( gurobi.optimal() )
    {
        Map<String, double> dontCare;
        gurobi.extractSolution( dontCare, newUb );
    }
    else if ( gurobi.timeout() )
    {
        newUb = gurobi.getObjectiveBound();
    }
    else
    {
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
    }

    Map<String, double> dontCare;
    gurobi.extractSolution( dontCare, newUb );

    // If the bound is tighter, store it
    if ( newUb < currentUb )
    {
        gurobi.setUpperBound( variableName, newUb );

        if ( FloatUtils::isPositive( currentUb ) &&
             !FloatUtils::isPositive( newUb ) )
            ++_signChanges;

        layer->setUb( neuron, newUb );
        _layerOwner->receiveTighterBound( Tightening( variable,
                                                      newUb,
                                                      Tightening::UB ) );
        ++_tighterBoundCounter;

        currentUb = newUb;

        if ( _cutoffInUse && newUb < _cutoffValue )
        {
            ++_cutoffs;
            return true;
        }
    }

    return false;
}

bool MILPFormulator::tightenLowerBound( GurobiWrapper &gurobi,
                                        Layer *layer,
                                        unsigned neuron,
                                        unsigned variable,
                                        double &currentLb )
{
    double newLb = FloatUtils::negativeInfinity();
    Stringf variableName( "x%u", variable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    gurobi.reset();
    gurobi.setCost( terms );
    gurobi.solve();

    if ( gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( gurobi.cutoffOccurred() )
    {
        newLb = _cutoffValue;
    }
    else if ( gurobi.optimal() )
    {
        Map<String, double> dontCare;
        gurobi.extractSolution( dontCare, newLb );
    }
    else if ( gurobi.timeout() )
    {
        newLb = gurobi.getObjectiveBound();
    }
    else
    {
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
    }

    // If the bound is tighter, store it
    if ( newLb > currentLb )
    {
        gurobi.setLowerBound( variableName, newLb );

        if ( FloatUtils::isNegative( currentLb ) &&
             !FloatUtils::isNegative( newLb ) )
            ++_signChanges;

        layer->setLb( neuron, newLb );
        _layerOwner->receiveTighterBound( Tightening( variable,
                                                      newLb,
                                                      Tightening::LB ) );
        ++_tighterBoundCounter;

        currentLb = newLb;

        if ( _cutoffInUse && newLb > _cutoffValue )
        {
            ++_cutoffs;
            return true;
        }
    }

    return false;
}

bool MILPFormulator::layerRequiresMILPEncoding( const Layer *layer )
{
    return layer->getLayerType() == Layer::RELU;
}

void MILPFormulator::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "Preprocessor: %s\n", message.ascii() );
}

} // namespace NLR
