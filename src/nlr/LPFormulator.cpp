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
#include "TimeUtils.h"

namespace NLR {

LPFormulator::LPFormulator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _cutoffInUse( false )
    , _cutoffValue( 0 )
{
    _gurobi.setTimeLimit( GlobalConfiguration::MILPSolverTimeoutValueInSeconds );
}

LPFormulator::~LPFormulator()
{
}

double LPFormulator::solveLPRelaxation( const Map<unsigned, Layer *> &layers,
                                        MinOrMax minOrMax,
                                        String variableName,
                                        unsigned lastLayer )
{
    _gurobi.resetModel();
    createLPRelaxation( layers, _gurobi, lastLayer );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    if ( minOrMax == MAX )
        _gurobi.setObjective( terms );
    else
        _gurobi.setCost( terms );

    _gurobi.solve();

    if ( _gurobi.infeasbile() )
        throw InfeasibleQueryException();

    if ( _gurobi.cutoffOccurred() )
        return _cutoffValue;

    if ( _gurobi.optimal() )
    {
        Map<String, double> dontCare;
        double result = 0;
        _gurobi.extractSolution( dontCare, result );
        return result;
    }
    else if ( _gurobi.timeout() )
    {
        return _gurobi.getObjectiveBound();
    }

    throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
}

void LPFormulator::optimizeBoundsWithIncrementalLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    _gurobi.resetModel();

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
        addLayerToModel( _gurobi, layer );

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
            _gurobi.reset();
            _gurobi.setObjective( terms );
            _gurobi.solve();

            if ( _gurobi.infeasbile() )
                throw InfeasibleQueryException();

            if ( _gurobi.cutoffOccurred() )
            {
                ub = _cutoffValue;
            }
            else if ( _gurobi.optimal() )
            {
                _gurobi.extractSolution( dontCare, ub );
            }
            else if ( _gurobi.timeout() )
            {
                ub = _gurobi.getObjectiveBound();
            }
            else
            {
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
            }

            // If the bound is tighter, store it
            if ( ub < currentUb )
            {
                _gurobi.setUpperBound( variableName, ub );

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
            _gurobi.reset();
            _gurobi.setCost( terms );
            _gurobi.solve();

            if ( _gurobi.infeasbile() )
                throw InfeasibleQueryException();

            if ( _gurobi.cutoffOccurred() )
            {
                lb = _cutoffValue;
            }
            else if ( _gurobi.optimal() )
            {
                _gurobi.extractSolution( dontCare, lb );
            }
            else if ( _gurobi.timeout() )
            {
                lb = _gurobi.getObjectiveBound();
            }
            else
            {
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
            }

            // If the bound is tighter, store it
            if ( lb > currentLb )
            {
                _gurobi.setLowerBound( variableName, lb );

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
    double lb = FloatUtils::negativeInfinity();
    double ub = FloatUtils::infinity();

    double currentLb;
    double currentUb;

    unsigned tighterBoundCounter = 0;
    unsigned signChanges = 0;
    unsigned cutoffs = 0;

    struct timespec gurobiStart;
    (void) gurobiStart;
    struct timespec gurobiEnd;
    (void) gurobiEnd;

    gurobiStart = TimeUtils::sampleMicro();

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

            unsigned variable = layer->neuronToVariable( i );
            Stringf variableName( "x%u", variable );

            ub = solveLPRelaxation( layers,
                                    MinOrMax::MAX,
                                    variableName,
                                    layer->getLayerIndex() );

            // Store the new bound if it is tighter
            if ( ub < currentUb )
            {
                if ( FloatUtils::isPositive( currentUb ) &&
                     !FloatUtils::isPositive( ub ) )
                    ++signChanges;

                layer->setUb( i, ub );
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

            lb = solveLPRelaxation( layers,
                                    MinOrMax::MIN,
                                    variableName,
                                    layer->getLayerIndex() );

            // Store the new bound if it is tighter
            if ( lb > currentLb )
            {
                if ( FloatUtils::isNegative( currentLb ) &&
                     !FloatUtils::isNegative( lb ) )
                    ++signChanges;

                layer->setLb( i, lb );
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

void LPFormulator::addReluLayerToLpRelaxation( GurobiWrapper &gurobi, const Layer *layer )
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
                        bias += weight * sourceLayer->getEliminatedNeuronValue( j );
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
