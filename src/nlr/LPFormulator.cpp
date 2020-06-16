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
{
}

LPFormulator::~LPFormulator()
{
}

double LPFormulator::solveLPRelaxation( const Map<unsigned, Layer *> &layers,
                                        MinOrMax minOrMax,
                                        String variableName,
                                        unsigned lastLayer )
{
    GurobiWrapper gurobi;
    createLPRelaxation( layers, gurobi, lastLayer );

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

void LPFormulator::optimizeBoundsWithIncrementalLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    GurobiWrapper gurobi;
    List<GurobiWrapper::Term> terms;
    Map<String, double> dontCare;
    double lb;
    double ub;

    unsigned tighterBoundCounter = 0;
    unsigned signChanges = 0;

    struct timespec gurobiStart = TimeUtils::sampleMicro();

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

            if ( !gurobi.optimal() )
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );

            gurobi.extractSolution( dontCare, ub );

            // Minimize
            gurobi.reset();
            gurobi.setCost( terms );
            gurobi.solve();
            if ( gurobi.infeasbile() )
                throw InfeasibleQueryException();

            if ( !gurobi.optimal() )
                throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );

            gurobi.extractSolution( dontCare, lb );

            // If the bounds are tighter, store them
            if ( lb > layer->getLb( j ) )
            {
                gurobi.setLowerBound( variableName, lb );

                if ( FloatUtils::isNegative( layer->getLb( j ) ) &&
                     !FloatUtils::isNegative( lb ) )
                    ++signChanges;

                layer->setLb( j, lb );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              lb,
                                                              Tightening::LB ) );
                ++tighterBoundCounter;
            }

            if ( ub < layer->getUb( j ) )
            {
                gurobi.setUpperBound( variableName, ub );

                if ( FloatUtils::isPositive( layer->getUb( j ) ) &&
                     !FloatUtils::isPositive( ub ) )
                    ++signChanges;

                layer->setUb( j, ub );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              ub,
                                                              Tightening::UB ) );
                ++tighterBoundCounter;
            }
        }
    }

    struct timespec gurobiEnd = TimeUtils::sampleMicro();

    log( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u\n",
                  tighterBoundCounter, signChanges ) );
    log( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ) );
}

void LPFormulator::optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    double lb = FloatUtils::negativeInfinity();
    double ub = FloatUtils::infinity();

    unsigned tighterBoundCounter = 0;
    unsigned signChanges = 0;

    struct timespec gurobiStart = TimeUtils::sampleMicro();

    for ( const auto &layer : layers )
    {
        for ( unsigned i = 0; i < layer.second->getSize(); ++i )
        {
            if ( layer.second->neuronEliminated( i ) )
                continue;

            unsigned variable = layer.second->neuronToVariable( i );
            Stringf variableName( "x%u", variable );

            lb = solveLPRelaxation( layers,
                                    MinOrMax::MIN,
                                    variableName,
                                    layer.second->getLayerIndex() );
            ub = solveLPRelaxation( layers,
                                    MinOrMax::MAX,
                                    variableName,
                                    layer.second->getLayerIndex() );

            // Store the new bounds if they are tighter
            if ( lb > layer.second->getLb( i ) )
            {
                if ( FloatUtils::isNegative( layer.second->getLb( i ) ) &&
                     !FloatUtils::isNegative( lb ) )
                    ++signChanges;

                layer.second->setLb( i, lb );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              lb,
                                                              Tightening::LB ) );
                ++tighterBoundCounter;
            }

            if ( ub < layer.second->getUb( i ) )
            {
                if ( FloatUtils::isPositive( layer.second->getUb( i ) ) &&
                     !FloatUtils::isPositive( ub ) )
                    ++signChanges;

                layer.second->setUb( i, ub );
                _layerOwner->receiveTighterBound( Tightening( variable,
                                                              ub,
                                                              Tightening::UB ) );
                ++tighterBoundCounter;
            }
        }
    }

    struct timespec gurobiEnd = TimeUtils::sampleMicro();

    log( Stringf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u\n",
                  tighterBoundCounter, signChanges ) );
    log( Stringf( "Seconds spent Gurobiing: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 ) );
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
        printf( "Unsupported layer!\n" );
        exit( 1 );
        break;
    }
}

void LPFormulator::addInputLayerToLpRelaxation( GurobiWrapper &gurobi,
                                                const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        unsigned varibale = layer->neuronToVariable( i );
        gurobi.addVariable( Stringf( "x%u", varibale ),
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

void LPFormulator::addWeightedSumLayerToLpRelaxation( GurobiWrapper &gurobi,
                                                      const Layer *layer )
{
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        if ( !layer->neuronEliminated( i ) )
        {
            unsigned varibale = layer->neuronToVariable( i );

            gurobi.addVariable( Stringf( "x%u", varibale ),
                                layer->getLb( i ),
                                layer->getUb( i ) );

            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", varibale ) ) );

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

void LPFormulator::log( const String &message )
{
    // if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "Preprocessor: %s\n", message.ascii() );
}

} // namespace NLR
