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
#include "TimeUtils.h"

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

    double newLb;
    double newUb;
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

            // Maximize, using just the LP relaxation for the current layer
            if ( tightenUpperBound( gurobi, layer, j, variable, currentUb, newUb ) )
                continue;

            // Minimize, using just the LP relaxation for the current layer
            if ( tightenLowerBound( gurobi, layer, j, variable, currentLb, newLb ) )
                continue;

            // Add the MILP encoding
            addLayerToModel( gurobi, layer );

            // Maximize, using just the exact MILP encoding
            if ( tightenUpperBound( gurobi, layer, j, variable, currentUb, newUb ) )
                continue;

            // Minimize, using just the exact MILP encoding
            if ( tightenLowerBound( gurobi, layer, j, variable, currentLb, newLb ) )
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
    _tighterBoundCounter = 0;
    _signChanges = 0;
    _cutoffs = 0;

    double newLb;
    double newUb;
    double currentLb;
    double currentUb;

    struct timespec gurobiStart = TimeUtils::sampleMicro();

    for ( const auto &currentLayer : layers )
    {
        Layer *layer = currentLayer.second;
        unsigned layerIndex = layer->getLayerIndex();

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
        for ( unsigned i = 0; i < layer->getSize(); ++i )
        {
            if ( layer->neuronEliminated( i ) )
                continue;

            currentLb = layer->getLb( i );
            currentUb = layer->getUb( i );

            if ( _cutoffInUse && ( currentLb > _cutoffValue || currentUb < _cutoffValue ) )
                continue;

            unsigned variable = layer->neuronToVariable( i );
            Stringf variableName( "x%u", variable );

            // LP relaxation, lower bound
            newLb = _lpFormulator.solveLPRelaxation( layers, LPFormulator::MIN, variableName, layerIndex );
            storeLbIfNeeded( layer, i, variable, newLb );
            if ( _cutoffInUse && newLb > _cutoffValue )
            {
                ++_cutoffs;
                continue;
            }

            // LP relaxation, upper bound
            newUb = _lpFormulator.solveLPRelaxation( layers, LPFormulator::MAX, variableName, layerIndex );
            storeUbIfNeeded( layer, i, variable, newUb );
            if ( _cutoffInUse && newUb < _cutoffValue )
            {
                ++_cutoffs;
                continue;
            }

            // MILP encoding, lower bound
            newLb = solveMILPEncoding( layers, MinOrMax::MIN, variableName, layerIndex );
            storeLbIfNeeded( layer, i, variable, newLb );
            if ( _cutoffInUse && newLb > _cutoffValue )
            {
                ++_cutoffs;
                continue;
            }

            // MILP encoding, upper bound
            newUb = solveMILPEncoding( layers, MinOrMax::MAX, variableName, layerIndex );
            storeUbIfNeeded( layer, i, variable, newUb );
            if ( _cutoffInUse && newUb < _cutoffValue )
            {
                ++_cutoffs;
                continue;
            }
        }
    }

    struct timespec gurobiEnd = TimeUtils::sampleMicro();

    log( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u. Cutoffs: %u\n",
                  _tighterBoundCounter, _signChanges, _cutoffs ) );
    log( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ) );
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

        addLayerToModel( gurobi, layer.second );
    }
}

void MILPFormulator::addLayerToModel( GurobiWrapper &gurobi, const Layer *layer )
{
    switch ( layer->getLayerType() )
    {
        case Layer::INPUT:
        case Layer::WEIGHTED_SUM:
            break;

        case Layer::RELU:
            addReluLayerToMILPFormulation( gurobi, layer );
            break;

        default:
            throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED, "MILPFormulator" );
            break;
    }
}

void MILPFormulator::addReluLayerToMILPFormulation( GurobiWrapper &gurobi,
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
            unsigned sourceVariable = sourceLayer->neuronToVariable( sourceNeuron );

            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            if ( !FloatUtils::isNegative( sourceLb ) ||
                 !FloatUtils::isPositive( sourceUb ) )
            {
                // This ReLU is fixed on one of its phases
                continue;
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
    }
}

double MILPFormulator::solveMILPEncoding( const Map<unsigned, Layer *> &layers,
                                          MinOrMax minOrMax,
                                          String variableName,
                                          unsigned lastLayer )
{
    GurobiWrapper gurobi;
    createMILPEncoding( layers, gurobi, lastLayer );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    if ( minOrMax == MAX )
        gurobi.setObjective( terms );
    else
        gurobi.setCost( terms );

    gurobi.solve();

    if ( gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( !gurobi.optimal() )
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );

    Map<String, double> dontCare;
    double result = 0;
    gurobi.extractSolution( dontCare, result );
    return result;
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
                                        double &currentUb,
                                        double &newUb )
{
    Stringf variableName( "x%u", variable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    gurobi.reset();
    gurobi.setObjective( terms );
    gurobi.solve();

    if ( gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( !gurobi.optimal() )
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );

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
                                        double &currentLb,
                                        double &newLb )
{
    Stringf variableName( "x%u", variable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    gurobi.reset();
    gurobi.setCost( terms );
    gurobi.solve();
    if ( gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( !gurobi.optimal() )
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );

    Map<String, double> dontCare;
    gurobi.extractSolution( dontCare, newLb );

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

void MILPFormulator::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "Preprocessor: %s\n", message.ascii() );
}

} // namespace NLR
