/*********************                                                        */
/*! \file DeepPolyAnalysis.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Debug.h"
#include "DeepPolyAnalysis.h"
#include "DeepPolyInputElement.h"
#include "DeepPolyWeightedSumElement.h"
#include "DeepPolyReLUElement.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Layer.h"
#include "MatrixMultiplication.h"
#include "MStringf.h"
#include "NLRError.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

DeepPolyAnalysis::DeepPolyAnalysis( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
{
}

DeepPolyAnalysis::~DeepPolyAnalysis()
{
    freeMemoryIfNeeded();
}

void DeepPolyAnalysis::freeMemoryIfNeeded()
{
}

void DeepPolyAnalysis::run( const Map<unsigned, Layer *> &layers )
{
    struct timespec deepPolyStart;
    (void) deepPolyStart;
    struct timespec deepPolyEnd;
    (void) deepPolyEnd;

    deepPolyStart = TimeUtils::sampleMicro();

    for ( unsigned i = 0; i <= _layerOwner->getNumberOfLayers(); ++i )
    {
        /*
          Go over the layers, one by one. Each time construct the abstract element
          and tighten the bounds
        */
        ASSERT( layers.exists( i ) );
        Layer *layer = layers[i];

        DeepPolyElement *deepPolyElement = createDeepPolyElement( layer );
        _deepPolyElements[i] = deepPolyElement;
        deepPolyElement->execute( _deepPolyElements );

        // Get the updated bound
        for ( unsigned j = 0; j <= deepPolyElement->getSize(); ++j )
        {
            double lb = deepPolyElement->getLowerBound( j );
            if ( layer->getLb( j ) < lb )
            {
                layer->setLb( j, lb );
                _layerOwner->receiveTighterBound
                    ( Tightening( layer->neuronToVariable( j ),
                                  lb, Tightening::LB ) );
            }
            double ub = deepPolyElement->getUpperBound( j );
            if ( layer->getUb( j ) > ub )
            {
                layer->setUb( j, ub );
                _layerOwner->receiveTighterBound
                    ( Tightening( layer->neuronToVariable( j ),
                                  ub, Tightening::UB ) );
            }

        }
    }
}

DeepPolyElement *DeepPolyAnalysis::createDeepPolyElement( Layer *layer )
{
    Layer::Type type = layer->getLayerType();
    DeepPolyElement *deepPolyElement;
    if ( type == Layer::INPUT )
        deepPolyElement = new DeepPolyInputElement( layer );
    else if ( type == Layer::WEIGHTED_SUM )
        deepPolyElement = new DeepPolyWeightedSumElement( layer );
    else if ( type ==  Layer::RELU )
        deepPolyElement = new DeepPolyReLUElement( layer );
    else
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED,
                        Stringf( "Layer %u not yet supported",
                                 layer->getLayerType() ).ascii() );
    return deepPolyElement;
}
    /*
void DeepPolyAnalysis::executeDeepPolyElementForInput( DeepPolyElement
                                                       &deepPolyElement )
{
    // Do not need to update symbolic bound, just update the concrete bounds
    ASSERT( deepPolyElement._layer->getLayerType() == Layer::INPUT );
    std::memcpy( deepPolyElement._lb, deepPolyElement._layer->getLbs(),
                 deepPolyElement._layer->getSize() );
    std::memcpy( deepPolyElement._ub, deepPolyElement._layer->getUbs(),
                 deepPolyElement._layer->getSize() );
}

void DeepPolyAnalysis::executeDeepPolyElementForWeightedSum( DeepPolyElement
                                                             &deepPolyElement )
{
    // Do not need to update symbolic bound, just update the concrete bounds
    ASSERT( deepPolyElement._layer->getLayerType() == Layer::WEIGHTED_SUM );
    computeBoundsWithBackSubstitution( deepPolyElement );
}

void DeepPolyAnalysis::computeBoundsWithBackSubstitution( DeepPolyElement
                                                          &deepPolyElement )
{

    for ( unsigned index = deepPolyElement._layerIndex - 1;
          index >=_startLayerIndex; --index )
    {
        if ( type == Layer::ReLU )
        {

        }
        if ( type == Layer::WEIGHTED_SUM )
        {


        }


    }
}

void DeepPolyAnalysis::executeDeepPolyElementForReLU( DeepPolyElement
                                                      &deepPolyElement )
{
}
    */

} // namespace NLR
