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

        double lb = linearLowerBound( sourceLbs, sourceUbs, index );
        double ub = linearUpperBound( sourceLbs, sourceUbs, index );
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
                        LSELowerBound( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj =
                            dLSELowerBound( sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }
                else
                {
                    _symbolicLowerBias[i] =
                        LSELowerBound2( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj =
                            dLSELowerBound2( sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }

                _symbolicUpperBias[i] = LSEUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj =
                        dLSEUpperbound( sourceMids, targetLbs, targetUbs, index, inputIndex );
                    _symbolicUb[_size * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
            else if ( _boundType == SoftmaxBoundType::EXPONENTIAL_RECIPROCAL_DECOMPOSITION )
            {
                _symbolicLowerBias[i] = ERLowerBound( sourceMids, sourceLbs, sourceUbs, index );
                unsigned inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dldj =
                        dERLowerBound( sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                    _symbolicLb[_size * sourceIndex._neuron + i] = dldj;
                    _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                    ++inputIndex;
                }

                _symbolicUpperBias[i] = ERUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj =
                        dERUpperBound( sourceMids, targetLbs, targetUbs, index, inputIndex );
                    _symbolicUb[_size * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
        }
    }
    log( "Executing - done" );
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

double DeepPolySoftmaxElement::LSELowerBound( const Vector<double> &inputs,
                                              const Vector<double> &inputLbs,
                                              const Vector<double> &inputUbs,
                                              unsigned i )
{
    double sum = 0;
    for ( unsigned j = 0; j < inputs.size(); ++j )
    {
        double lj = inputLbs[j];
        double uj = inputUbs[j];
        double xj = inputs[j];
        sum +=
            ( uj - xj ) / ( uj - lj ) * std::exp( lj ) + ( xj - lj ) / ( uj - lj ) * std::exp( uj );
    }

    return std::exp( inputs[i] ) / sum;
}

double DeepPolySoftmaxElement::dLSELowerBound( const Vector<double> &inputMids,
                                               const Vector<double> &inputLbs,
                                               const Vector<double> &inputUbs,
                                               unsigned i,
                                               unsigned di )
{
    double val = 0;
    if ( i == di )
        val += LSELowerBound( inputMids, inputLbs, inputUbs, i );

    double ldi = inputLbs[di];
    double udi = inputUbs[di];

    double sum = 0;
    for ( unsigned j = 0; j < inputMids.size(); ++j )
    {
        double lj = inputLbs[j];
        double uj = inputUbs[j];
        double xj = inputMids[j];

        sum +=
            ( uj - xj ) / ( uj - lj ) * std::exp( lj ) + ( xj - lj ) / ( uj - lj ) * std::exp( uj );
    }

    val -= std::exp( inputMids[i] ) / ( sum * sum ) * ( std::exp( udi ) - std::exp( ldi ) ) /
           ( udi - ldi );

    return val;
}

double DeepPolySoftmaxElement::LSELowerBound2( const Vector<double> &inputMids,
                                               const Vector<double> &inputLbs,
                                               const Vector<double> &inputUbs,
                                               unsigned i )
{
    double max = FloatUtils::negativeInfinity();
    unsigned maxInputIndex = 0;
    unsigned index = 0;
    for ( const auto &mid : inputMids )
    {
        if ( mid > max )
        {
            max = mid;
            maxInputIndex = index;
        }
        ++index;
    }

    if ( maxInputIndex == i )
        return ERLowerBound( inputMids, inputLbs, inputUbs, i );
    else
    {
        double sum = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j == maxInputIndex )
                sum += 1;
            else
            {
                double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                double xjjstar = inputMids[j] - inputMids[maxInputIndex];

                sum += ( ujjstar - xjjstar ) / ( ujjstar - ljjstar ) * std::exp( ljjstar ) +
                       ( xjjstar - ljjstar ) / ( ujjstar - ljjstar ) * std::exp( ujjstar );
            }
        }

        return std::exp( inputMids[i] - inputMids[maxInputIndex] ) / sum;
    }
}

double DeepPolySoftmaxElement::dLSELowerBound2( const Vector<double> &inputMids,
                                                const Vector<double> &inputLbs,
                                                const Vector<double> &inputUbs,
                                                unsigned i,
                                                unsigned di )
{
    double max = FloatUtils::negativeInfinity();
    unsigned maxInputIndex = 0;
    unsigned index = 0;
    for ( const auto &mid : inputMids )
    {
        if ( mid > max )
        {
            max = mid;
            maxInputIndex = index;
        }
        ++index;
    }

    if ( maxInputIndex == i )
        return dERLowerBound( inputMids, inputLbs, inputUbs, i, di );
    else
    {
        double val = LSELowerBound2( inputMids, inputLbs, inputUbs, i );

        double sum = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j == maxInputIndex )
                sum += 1;
            else
            {
                double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                double xjjstar = inputMids[j] - inputMids[maxInputIndex];
                sum += ( ujjstar - xjjstar ) / ( ujjstar - ljjstar ) * std::exp( ljjstar ) +
                       ( xjjstar - ljjstar ) / ( ujjstar - ljjstar ) * std::exp( ujjstar );
            }
        }
        double val2 = std::exp( inputMids[i] - inputMids[maxInputIndex] ) / ( sum * sum );

        if ( i == di )
        {
            double ldijstar = inputLbs[i] - inputUbs[maxInputIndex];
            double udijstar = inputUbs[i] - inputLbs[maxInputIndex];
            return val -
                   val2 * ( std::exp( udijstar ) - std::exp( ldijstar ) ) / ( udijstar - ldijstar );
        }
        else if ( maxInputIndex == di )
        {
            double sum2 = 0;
            for ( unsigned j = 0; j < inputMids.size(); ++j )
            {
                if ( j == maxInputIndex )
                    continue;
                else
                {
                    double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                    double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                    sum2 += ( std::exp( ujjstar ) - std::exp( ljjstar ) ) / ( ujjstar - ljjstar );
                }
            }
            return -val + val2 * sum2;
        }
        else
        {
            double ldijstar = inputLbs[di] - inputUbs[maxInputIndex];
            double udijstar = inputUbs[di] - inputLbs[maxInputIndex];
            return -val2 * ( std::exp( udijstar ) - std::exp( ldijstar ) ) /
                   ( udijstar - ldijstar );
        }
    }
}

double DeepPolySoftmaxElement::LSEUpperBound( const Vector<double> &inputs,
                                              const Vector<double> &outputLb,
                                              const Vector<double> &outputUb,
                                              unsigned i )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    return ( ( li * std::log( ui ) - ui * std::log( li ) ) / ( std::log( ui ) - std::log( li ) ) -
             ( ui - li ) / ( std::log( ui ) - std::log( li ) ) *
                 SoftmaxConstraint::logSumOfExponential( inputTilda ) );
}

double DeepPolySoftmaxElement::dLSEUpperbound( const Vector<double> &inputMids,
                                               const Vector<double> &outputLb,
                                               const Vector<double> &outputUb,
                                               unsigned i,
                                               unsigned di )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    double val = -( ui - li ) / ( std::log( ui ) - std::log( li ) );

    double val2 = std::exp( inputMids[di] ) / SoftmaxConstraint::sumOfExponential( inputMids );
    if ( i == di )
        val2 -= 1;

    return val * val2;
}

double DeepPolySoftmaxElement::ERLowerBound( const Vector<double> &inputs,
                                             const Vector<double> &inputLbs,
                                             const Vector<double> &inputUbs,
                                             unsigned i )
{
    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    double sum = 0;
    for ( unsigned j = 0; j < inputs.size(); ++j )
    {
        if ( i == j )
            sum += 1;
        else
        {
            double ljTilda = inputLbs[j] - inputUbs[i];
            double ujTilda = inputUbs[j] - inputLbs[i];
            double xjTilda = inputTilda[j];

            sum += ( ujTilda - xjTilda ) / ( ujTilda - ljTilda ) * std::exp( ljTilda ) +
                   ( xjTilda - ljTilda ) / ( ujTilda - ljTilda ) * std::exp( ujTilda );
        }
    }

    return 1 / sum;
}

double DeepPolySoftmaxElement::dERLowerBound( const Vector<double> &inputMids,
                                              const Vector<double> &inputLbs,
                                              const Vector<double> &inputUbs,
                                              unsigned i,
                                              unsigned di )
{
    double val = ERLowerBound( inputMids, inputLbs, inputUbs, i );

    if ( i != di )
    {
        double ldiTilda = inputLbs[di] - inputUbs[i];
        double udiTilda = inputUbs[di] - inputLbs[i];
        return -val * val * ( std::exp( udiTilda ) - std::exp( ldiTilda ) ) /
               ( udiTilda - ldiTilda );
    }
    else
    {
        double val2 = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j != i )
            {
                double ljTilda = inputLbs[j] - inputUbs[i];
                double ujTilda = inputUbs[j] - inputLbs[i];
                val2 += ( std::exp( ujTilda ) - std::exp( ljTilda ) ) / ( ujTilda - ljTilda );
            }
        }
        return val * val * val2;
    }
}

double DeepPolySoftmaxElement::ERUpperBound( const Vector<double> &inputs,
                                             const Vector<double> &outputLb,
                                             const Vector<double> &outputUb,
                                             unsigned i )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    return ui + li - ui * li * SoftmaxConstraint::sumOfExponential( inputTilda );
}

double DeepPolySoftmaxElement::dERUpperBound( const Vector<double> &inputMids,
                                              const Vector<double> &outputLb,
                                              const Vector<double> &outputUb,
                                              unsigned i,
                                              unsigned di )
{
    double li = outputLb[i];
    double ui = outputUb[i];


    if ( i == di )
    {
        double val2 = -1;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
            val2 += std::exp( inputMids[j] - inputMids[i] );
        return li * ui * val2;
    }
    else
        return -li * ui * std::exp( inputMids[di] - inputMids[i] );
}

double DeepPolySoftmaxElement::linearLowerBound( const Vector<double> &inputLbs,
                                                 const Vector<double> &inputUbs,
                                                 unsigned i )
{
    Vector<double> uTilda;
    SoftmaxConstraint::xTilda( inputUbs, inputLbs[i], uTilda );
    uTilda[i] = 0;
    return 1 / SoftmaxConstraint::sumOfExponential( uTilda );
}

double DeepPolySoftmaxElement::linearUpperBound( const Vector<double> &inputLbs,
                                                 const Vector<double> &inputUbs,
                                                 unsigned i )
{
    Vector<double> lTilda;
    SoftmaxConstraint::xTilda( inputLbs, inputUbs[i], lTilda );
    lTilda[i] = 0;
    return 1 / SoftmaxConstraint::sumOfExponential( lTilda );
}

void DeepPolySoftmaxElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolySoftmaxElement: %s\n", message.ascii() );
}

} // namespace NLR
