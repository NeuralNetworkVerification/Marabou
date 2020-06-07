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
#include "LPFormulator.h"
#include "Layer.h"
#include "MStringf.h"
#include "TimeUtils.h"

namespace NLR {

LPFormulator::LPFormulator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
{
}

LPFormulator::~LPFormulator()
{
}

void LPFormulator::optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    List<GurobiWrapper::Term> terms;
    Map<String, double> dontCare;
    double lb, ub;

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

            {
                // Maximize
                GurobiWrapper gurobi;
                createLPRelaxation( layers, gurobi, layer.first );
                terms.clear();
                terms.append( GurobiWrapper::Term( 1, variableName ) );
                gurobi.setObjective( terms );
                gurobi.solve();
                gurobi.extractSolution( dontCare, ub );
            }

            {
                // Minimize
                GurobiWrapper gurobi;
                createLPRelaxation( layers, gurobi, layer.first );
                terms.clear();
                terms.append( GurobiWrapper::Term( 1, variableName ) );
                gurobi.setCost( terms );
                gurobi.solve();
                gurobi.extractSolution( dontCare, lb );
            }

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

    printf( "Number of tighter bounds found by Gurobi: %u. Sign changes: %u\n",
            tighterBoundCounter, signChanges );

    struct timespec gurobiEnd = TimeUtils::sampleMicro();
    printf( "Seconds spent Gurobi'ing seconds: %llu\n", TimeUtils::timePassed( gurobiStart, gurobiEnd ) / 1000000 );
}

void LPFormulator::createLPRelaxation( const Map<unsigned, Layer *> &layers,
                                       GurobiWrapper &gurobi,
                                       unsigned lastLayer )
{
    for ( const auto &layer : layers )
    {
        if ( layer.first > lastLayer )
            return;

        switch ( layer.second->getLayerType() )
        {
        case Layer::INPUT:
            addInputLayerToLpRelaxation( gurobi, layer.second );
            break;

        case Layer::RELU:
            addReluLayerToLpRelaxation( gurobi, layer.second );
            break;

        case Layer::WEIGHTED_SUM:
        case Layer::OUTPUT:
            addWeightedSumLayerToLpRelaxation( gurobi, layer.second );
            break;

        default:
            printf( "Unsupported layer!\n" );
            exit( 1 );
            break;
        }
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
                gurobi.addGeqConstraint( terms, 0 );
            }
            else if ( !FloatUtils::isPositive( sourceUb ) )
            {
                // The ReLU is inactive, y = 0
                List<GurobiWrapper::Term> terms;
                terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
                gurobi.addGeqConstraint( terms, 0 );
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
                gurobi.addLeqConstraint( terms, ( -sourceUb * sourceLb ) / ( sourceUb - sourceLb ));
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

} // namespace NLR
