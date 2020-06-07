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

namespace NLR {

LPFormulator::LPFormulator( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
{
}

LPFormulator::~LPFormulator()
{
}

void LPFormulator::createLPRelaxation( const Map<unsigned, Layer *> &layers )
{
    GurobiWrapper gurobi;

    for ( const auto &layer : layers )
    {
        switch ( layer.second->getLayerType() )
        {
        case Layer::INPUT:
            addInputLayerToLpRelaxation( gurobi, layer.second );
            break;

        case Layer::RELU:
            addReluLayerToLpRelaxation();
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

void LPFormulator::addReluLayerToLpRelaxation()
{
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
            double bias = layer->getBias( i );

            for ( const auto &sourceLayerPair : layer->getSourceLayers() )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerPair.first );
                unsigned sourceLayerSize = sourceLayerPair.second;

                for ( unsigned j = 0; i < sourceLayerSize; ++j )
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

} // namespace NLR
