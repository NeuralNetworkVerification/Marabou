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

void LPFormulator::optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers )
{
    GurobiWrapper gurobi;
    createLPRelaxation( layers, gurobi );
    printf( "Done creating relaxation, dumping model\n" );

    gurobi.dump();

}

void LPFormulator::createLPRelaxation( const Map<unsigned, Layer *> &layers,
                                       GurobiWrapper &gurobi )
{
    for ( const auto &layer : layers )
    {
        printf( "LP Formulator: starting work on layer %u\n", layer.second->getLayerIndex() );

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
        printf( "\tNeuron: <%u, %u>\n", layer->getLayerIndex(), i );

        if ( !layer->neuronEliminated( i ) )
        {
            printf( "\t\t not eliminated\n" );

            unsigned varibale = layer->neuronToVariable( i );

            printf( "\t\tVariable is: %u\n", varibale );

            gurobi.addVariable( Stringf( "x%u", varibale ),
                                layer->getLb( i ),
                                layer->getUb( i ) );

            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", varibale ) ) );

            double bias = -layer->getBias( i );

            printf( "\t\tBias is: %.5lf\n", bias );

            for ( const auto &sourceLayerPair : layer->getSourceLayers() )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerPair.first );
                unsigned sourceLayerSize = sourceLayerPair.second;

                printf( "\t\tHandling source layer %u, size %u\n", sourceLayer->getLayerIndex(), sourceLayerSize );

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
