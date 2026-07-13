/*********************                                                        */
/*! \file DeepPolySoftmaxElement.cpp
** \verbatim
** Top contributors (to current version):
**   Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#include "DeepPolySoftmaxElement.h"

#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"
#include "SoftmaxConstraint.h"

#include <string.h>

namespace NLR {

DeepPolySoftmaxElement::DeepPolySoftmaxElement( Layer *layer, unsigned maxLayerSize )
    : _boundType( Options::get()->getSoftmaxBoundType() )
    , _maxLayerSize( maxLayerSize )
    , _work( NULL )
{
    log( Stringf( "Softmax bound type: %s",
                  Options::get()->getString( Options::SOFTMAX_BOUND_TYPE ).ascii() ) );
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolySoftmaxElement::~DeepPolySoftmaxElement()
{
    freeMemoryIfNeeded();
}

void DeepPolySoftmaxElement::execute(
    const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory( _maxLayerSize );
    getConcreteBounds();

    // This function rely on the assumptions described in the
    // constructSoftmaxLayer() method in the Query class

    Set<unsigned> handledInputNeurons;
    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        List<NeuronIndex> sources = _layer->getActivationSources( i );

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        Vector<double> sourceMids;
        Vector<double> targetLbs;
        Vector<double> targetUbs;
        for ( const auto &sourceIndex : sources )
        {
            DeepPolyElement *predecessor = deepPolyElementsBefore[sourceIndex._layer];
            double sourceLb = predecessor->getLowerBound( sourceIndex._neuron );
            sourceLbs.append( sourceLb - GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
            double sourceUb = predecessor->getUpperBound( sourceIndex._neuron );
            sourceUbs.append( sourceUb + GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
            sourceMids.append( ( sourceLb + sourceUb ) / 2 );
            targetLbs.append( _lb[i] );
            targetUbs.append( _ub[i] );
        }

        // Find the index of i in the softmax
        unsigned index = 0;
        for ( const auto &sourceIndex : sources )
        {
            if ( handledInputNeurons.exists( sourceIndex._neuron ) )
                ++index;
            else
            {
                handledInputNeurons.insert( sourceIndex._neuron );
                break;
            }
        }

        double lb = Layer::linearLowerBound( sourceLbs, sourceUbs, index );
        double ub = Layer::linearUpperBound( sourceLbs, sourceUbs, index );
        if ( lb > _lb[i] )
            _lb[i] = lb;
        if ( ub < _ub[i] )
            _ub[i] = ub;
        log( Stringf( "Current bounds of neuron %u: [%f, %f]", i, _lb[i], _ub[i] ) );
        targetLbs[index] = _lb[i];
        targetUbs[index] = _ub[i];

        if ( FloatUtils::areEqual( _lb[i], _ub[i] ) )
        {
            _symbolicLowerBias[i] = _lb[i];
            _symbolicUpperBias[i] = _ub[i];
            for ( const auto &sourceIndex : sources )
            {
                _symbolicLb[_size * sourceIndex._neuron + i] = 0;
                _symbolicUb[_size * sourceIndex._neuron + i] = 0;
            }
        }
        else
        {
            // Compute symbolic bound
            if ( _boundType == SoftmaxBoundType::LOG_SUM_EXP_DECOMPOSITION )
            {
                bool useLSE2 = false;
                for ( const auto &lb : targetLbs )
                {
                    if ( lb > GlobalConfiguration::SOFTMAX_LSE2_THRESHOLD )
                        useLSE2 = true;
                }
                unsigned inputIndex = 0;
                if ( !useLSE2 )
                {
                    _symbolicLowerBias[i] =
                        Layer::LSELowerBound( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj = Layer::dLSELowerBound(
                            sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }
                else
                {
                    _symbolicLowerBias[i] =
                        Layer::LSELowerBound2( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj = Layer::dLSELowerBound2(
                            sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }

                _symbolicUpperBias[i] =
                    Layer::LSEUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj = Layer::dLSEUpperbound(
                        sourceMids, targetLbs, targetUbs, index, inputIndex );
                    _symbolicUb[_size * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
            else if ( _boundType == SoftmaxBoundType::EXPONENTIAL_RECIPROCAL_DECOMPOSITION )
            {
                _symbolicLowerBias[i] =
                    Layer::ERLowerBound( sourceMids, sourceLbs, sourceUbs, index );
                unsigned inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dldj =
                        Layer::dERLowerBound( sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                    _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                    _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                    ++inputIndex;
                }

                _symbolicUpperBias[i] =
                    Layer::ERUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj =
                        Layer::dERUpperBound( sourceMids, targetLbs, targetUbs, index, inputIndex );
                    _symbolicUb[_size * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
        }
    }

    if ( _storePredecessorSymbolicBounds )
    {
        storePredecessorSymbolicBounds();
    }

    log( "Executing - done" );
}

void DeepPolySoftmaxElement::storePredecessorSymbolicBounds()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        List<NeuronIndex> sources = _layer->getActivationSources( i );
        unsigned inputIndex = 0;
        for ( const auto &sourceIndex : sources )
        {
            ( *_predecessorSymbolicLb )[_layerIndex][_size * inputIndex + i] =
                _symbolicLb[_size * sourceIndex._neuron + i];
            ( *_predecessorSymbolicUb )[_layerIndex][_size * inputIndex + i] =
                _symbolicUb[_size * sourceIndex._neuron + i];
            ++inputIndex;
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        ( *_predecessorSymbolicLowerBias )[_layerIndex][i] = _symbolicLowerBias[i];
        ( *_predecessorSymbolicUpperBias )[_layerIndex][i] = _symbolicUpperBias[i];
    }
}

void DeepPolySoftmaxElement::symbolicBoundInTermsOfPredecessor(
    const double *symbolicLb,
    const double *symbolicUb,
    double *symbolicLowerBias,
    double *symbolicUpperBias,
    double *symbolicLbInTermsOfPredecessor,
    double *symbolicUbInTermsOfPredecessor,
    unsigned targetLayerSize,
    DeepPolyElement *predecessor )
{
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessor->getLayerIndex() ) );

    unsigned predecessorSize = predecessor->getSize();
    ASSERT( predecessorSize == _size );

    for ( unsigned i = 0; i < _size * targetLayerSize; ++i )
    {
        if ( symbolicLb[i] > 0 )
            _work[i] = symbolicLb[i];
        else
            _work[i] = 0;
    }
    // _work is now positive weights in symbolicLb
    matrixMultiplication( _symbolicLb,
                          _work,
                          symbolicLbInTermsOfPredecessor,
                          predecessorSize,
                          _size,
                          targetLayerSize );
    if ( symbolicLowerBias )
        matrixMultiplication(
            _symbolicLowerBias, _work, symbolicLowerBias, 1, _size, targetLayerSize );

    for ( unsigned i = 0; i < _size * targetLayerSize; ++i )
    {
        if ( symbolicLb[i] < 0 )
            _work[i] = symbolicLb[i];
        else
            _work[i] = 0;
    }
    // _work is now negative weights in symbolicLb
    matrixMultiplication( _symbolicUb,
                          _work,
                          symbolicLbInTermsOfPredecessor,
                          predecessorSize,
                          _size,
                          targetLayerSize );
    if ( symbolicLowerBias )
        matrixMultiplication(
            _symbolicUpperBias, _work, symbolicLowerBias, 1, _size, targetLayerSize );

    for ( unsigned i = 0; i < _size * targetLayerSize; ++i )
    {
        if ( symbolicUb[i] > 0 )
            _work[i] = symbolicUb[i];
        else
            _work[i] = 0;
    }
    // _work is now positive weights in symbolicUb
    matrixMultiplication( _symbolicUb,
                          _work,
                          symbolicUbInTermsOfPredecessor,
                          predecessorSize,
                          _size,
                          targetLayerSize );
    if ( symbolicUpperBias )
        matrixMultiplication(
            _symbolicUpperBias, _work, symbolicUpperBias, 1, _size, targetLayerSize );

    for ( unsigned i = 0; i < _size * targetLayerSize; ++i )
    {
        if ( symbolicUb[i] < 0 )
            _work[i] = symbolicUb[i];
        else
            _work[i] = 0;
    }
    // _work is now positive weights in symbolicUb
    matrixMultiplication( _symbolicLb,
                          _work,
                          symbolicUbInTermsOfPredecessor,
                          predecessorSize,
                          _size,
                          targetLayerSize );
    if ( symbolicUpperBias )
        matrixMultiplication(
            _symbolicLowerBias, _work, symbolicUpperBias, 1, _size, targetLayerSize );

    log( Stringf( "Computing symbolic bounds with respect to layer %u - done",
                  predecessor->getLayerIndex() ) );
}

void DeepPolySoftmaxElement::allocateMemory( unsigned maxLayerSize )
{
    freeMemoryIfNeeded();

    DeepPolyElement::allocateMemory();

    unsigned size = _size * _size;
    _symbolicLb = new double[size];
    _symbolicUb = new double[size];

    std::fill_n( _symbolicLb, size, 0 );
    std::fill_n( _symbolicUb, size, 0 );

    _symbolicLowerBias = new double[_size];
    _symbolicUpperBias = new double[_size];

    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );

    _work = new double[_size * maxLayerSize];
    std::fill_n( _work, _size * maxLayerSize, 0 );
}

void DeepPolySoftmaxElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
    if ( _symbolicLb )
    {
        delete[] _symbolicLb;
        _symbolicLb = NULL;
    }
    if ( _symbolicUb )
    {
        delete[] _symbolicUb;
        _symbolicUb = NULL;
    }
    if ( _symbolicLowerBias )
    {
        delete[] _symbolicLowerBias;
        _symbolicLowerBias = NULL;
    }
    if ( _symbolicUpperBias )
    {
        delete[] _symbolicUpperBias;
        _symbolicUpperBias = NULL;
    }
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }
}

void DeepPolySoftmaxElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolySoftmaxElement: %s\n", message.ascii() );
}

} // namespace NLR
