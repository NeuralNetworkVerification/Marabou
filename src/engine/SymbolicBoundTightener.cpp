/*********************                                                        */
/*! \file SymbolicBoundTightener.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "SymbolicBoundTightener.h"

SymbolicBoundTightener::SymbolicBoundTightener()
    : _layerSizes( NULL )
    , _biases( NULL )
    , _weights( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _currentLayerLowerBounds( NULL )
    , _currentLayerUpperBounds( NULL )
    , _currentLayerLowerBias( NULL )
    , _currentLayerUpperBias( NULL )
    , _previousLayerLowerBounds( NULL )
    , _previousLayerUpperBounds( NULL )
    , _previousLayerLowerBias( NULL )
    , _previousLayerUpperBias( NULL )
{
}

SymbolicBoundTightener::~SymbolicBoundTightener()
{
    freeMemoryIfNeeded();
}

void SymbolicBoundTightener::freeMemoryIfNeeded()
{
    if ( _biases )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _biases[i] )
            {
                delete[] _biases[i];
                _biases[i] = NULL;
            }
        }

        delete[] _biases;
        _biases = NULL;
    }

    if ( _weights )
    {
        for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        {
            if ( _weights[i]._positiveValues )
            {
                delete[] _weights[i]._positiveValues;
                _weights[i]._positiveValues = NULL;
            }

            if ( _weights[i]._negativeValues )
            {
                delete[] _weights[i]._negativeValues;
                _weights[i]._negativeValues = NULL;
            }
        }

        delete[] _weights;
        _weights = NULL;
    }

    if ( _lowerBounds )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _lowerBounds[i] )
            {
                delete[] _lowerBounds[i];
                _lowerBounds[i] = NULL;
            }
        }

        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _upperBounds[i] )
            {
                delete[] _upperBounds[i];
                _upperBounds[i] = NULL;
            }
        }

        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _currentLayerLowerBounds )
    {
        delete[] _currentLayerLowerBounds;
        _currentLayerLowerBounds = NULL;
    }

    if ( _currentLayerUpperBounds )
    {
        delete[] _currentLayerUpperBounds;
        _currentLayerUpperBounds = NULL;
    }

    if ( _currentLayerLowerBias )
    {
        delete[] _currentLayerLowerBias;
        _currentLayerLowerBias = NULL;
    }

    if ( _currentLayerUpperBias )
    {
        delete[] _currentLayerUpperBias;
        _currentLayerUpperBias = NULL;
    }

    if ( _previousLayerLowerBounds )
    {
        delete[] _previousLayerLowerBounds;
        _previousLayerLowerBounds = NULL;
    }

    if ( _previousLayerUpperBounds )
    {
        delete[] _previousLayerUpperBounds;
        _previousLayerUpperBounds = NULL;
    }

    if ( _previousLayerLowerBias )
    {
        delete[] _previousLayerLowerBias;
        _previousLayerLowerBias = NULL;
    }

    if ( _previousLayerUpperBias )
    {
        delete[] _previousLayerUpperBias;
        _previousLayerUpperBias = NULL;
    }

    if ( _layerSizes )
    {
        delete[] _layerSizes;
        _layerSizes = NULL;
    }

    _inputLowerBounds.clear();
    _inputUpperBounds.clear();
}

void SymbolicBoundTightener::setNumberOfLayers( unsigned layers )
{
    freeMemoryIfNeeded();

    _numberOfLayers = layers;
    _layerSizes = new unsigned[layers];

    std::fill_n( _layerSizes, layers, 0 );
}

void SymbolicBoundTightener::setLayerSize( unsigned layer, unsigned layerSize )
{
    _layerSizes[layer] = layerSize;
}

void SymbolicBoundTightener::allocateWeightAndBiasSpace()
{
    // Allocate biases
    _biases = new double *[_numberOfLayers];
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
    {
        ASSERT( _layerSizes[i] > 0 );
        _biases[i] = new double[_layerSizes[i]];

        std::fill_n( _biases[i], _layerSizes[i], 0 );
    }

    // Allocate weights
    _weights = new WeightMatrix[_numberOfLayers - 1];
    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        // The rows represent the sources, the columns the targets
        _weights[i]._rows = _layerSizes[i];
        _weights[i]._columns = _layerSizes[i+1];
        _weights[i]._positiveValues = new double[_weights[i]._rows * _weights[i]._columns];
        _weights[i]._negativeValues = new double[_weights[i]._rows * _weights[i]._columns];

        std::fill_n( _weights[i]._positiveValues, _weights[i]._rows * _weights[i]._columns, 0 );
        std::fill_n( _weights[i]._negativeValues, _weights[i]._rows * _weights[i]._columns, 0 );
    }

    _lowerBounds = new double *[_numberOfLayers];
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
    {
        ASSERT( _layerSizes[i] > 0 );
        _lowerBounds[i] = new double[_layerSizes[i]];

        std::fill_n( _lowerBounds[i], _layerSizes[i], 0 );
    }

    _upperBounds = new double *[_numberOfLayers];
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
    {
        ASSERT( _layerSizes[i] > 0 );
        _upperBounds[i] = new double[_layerSizes[i]];

        std::fill_n( _upperBounds[i], _layerSizes[i], FloatUtils::infinity() );
    }

    // Allocate work space for the bound computation
    _maxLayerSize = 0;
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
        if ( _maxLayerSize < _layerSizes[i] )
            _maxLayerSize = _layerSizes[i];

    _inputLayerSize = _layerSizes[0];

    _currentLayerLowerBounds = new double[_maxLayerSize * _inputLayerSize];
    _currentLayerUpperBounds = new double[_maxLayerSize * _inputLayerSize];
    _currentLayerLowerBias = new double[_maxLayerSize];
    _currentLayerUpperBias = new double[_maxLayerSize];

    _previousLayerLowerBounds = new double[_maxLayerSize * _inputLayerSize];
    _previousLayerUpperBounds = new double[_maxLayerSize * _inputLayerSize];
    _previousLayerLowerBias = new double[_maxLayerSize];
    _previousLayerUpperBias = new double[_maxLayerSize];
}

void SymbolicBoundTightener::setBias( unsigned layer, unsigned neuron, double bias )
{
    _biases[layer][neuron] = bias;
}

void SymbolicBoundTightener::setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
{
    if ( weight > 0 )
        _weights[sourceLayer]._positiveValues[sourceNeuron * _weights[sourceLayer]._columns + targetNeuron] = weight;
    else
        _weights[sourceLayer]._negativeValues[sourceNeuron * _weights[sourceLayer]._columns + targetNeuron] = weight;
}

void SymbolicBoundTightener::setInputLowerBound( unsigned neuron, double bound )
{
    _inputLowerBounds[neuron] = bound;
}

void SymbolicBoundTightener::setInputUpperBound( unsigned neuron, double bound )
{
    _inputUpperBounds[neuron] = bound;
}

void SymbolicBoundTightener::run()
{
    /*
      Initialize the symbolic bounds for the first layer. Each variable has symbolic
      upper and lower bound 1 for itself, 0 for all other varibales.
      The input layer has no biases.
    */
    std::fill_n( _previousLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
    std::fill_n( _previousLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );
    for ( unsigned i = 0; i < _inputLayerSize; ++i )
    {
        _previousLayerLowerBounds[i * _inputLayerSize + i] = 1;
        _previousLayerUpperBounds[i * _inputLayerSize + i] = 1;
    }
    std::fill_n( _previousLayerLowerBias, _maxLayerSize, 0 );
    std::fill_n( _previousLayerUpperBias, _maxLayerSize, 0 );

    log( "Initializing.\n\tLB matrix:\n" );
    for ( unsigned i = 0; i < _inputLayerSize; ++i )
    {
        log( "\t" );
        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            log( Stringf( "%.2lf ", _previousLayerLowerBounds[i*_inputLayerSize + j] ) );
        }
        log( "\n" );
    }
    log( "\nUB matrix:\n" );
    for ( unsigned i = 0; i < _inputLayerSize; ++i )
    {
        log( "\t" );
        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            log( Stringf( "%.2lf ", _previousLayerUpperBounds[i*_inputLayerSize + j] ) );
        }
        log( "\n" );
    }

    for ( unsigned currentLayer = 1; currentLayer < _numberOfLayers; ++currentLayer )
    {
        log( Stringf( "\nStarting work on layer %u\n", currentLayer ) );

        unsigned currentLayerSize = _layerSizes[currentLayer];
        unsigned previousLayerSize = _layerSizes[currentLayer - 1];

        /*
          Computing symbolic bounds for layer i, based on layer i-1.
          We assume that the bounds for the previous layer have been computed.
        */

        std::fill_n( _currentLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
        std::fill_n( _currentLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );

        // Grab the weights
        WeightMatrix weights = _weights[currentLayer-1];

        log( "Positive weights:\n" );
        for ( unsigned i = 0; i < _layerSizes[currentLayer - 1]; ++i )
        {
            log( "\t" );
            for ( unsigned j = 0; j < _layerSizes[currentLayer]; ++j )
            {
                log( Stringf( "%.2lf ", weights._positiveValues[i*_layerSizes[currentLayer] + j] ) );
            }
            log( "\n" );
        }
        log( "\nNegative weights:\n" );
        for ( unsigned i = 0; i < _layerSizes[currentLayer - 1]; ++i )
        {
            log( "\t" );
            for ( unsigned j = 0; j < _layerSizes[currentLayer]; ++j )
            {
                log( Stringf( "%.2lf ", weights._negativeValues[i*_layerSizes[currentLayer] + j] ) );
            }
            log( "\n" );
        }

        /*
          Perform the multiplication.

            newUB = oldUB * posWeights + oldLB * negWeights
            newLB = oldUB * negWeights + oldLB * posWeights

            dimensions for oldUB and oldLB: inputLayerSize x previousLayerSize
            dimensions for posWeights and negWeights: previousLayerSize x layerSize

            newUB, newLB dimensions: inputLayerSize x layerSize
        */

        for ( unsigned i = 0; i < _inputLayerSize; ++i )
        {
            for ( unsigned j = 0; j < currentLayerSize; ++j )
            {
                for ( unsigned k = 0; k < previousLayerSize; ++k )
                {
                    _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        _previousLayerUpperBounds[i * previousLayerSize + k] *
                        weights._negativeValues[k * currentLayerSize + j];

                    _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        _previousLayerLowerBounds[i * previousLayerSize + k] *
                        weights._positiveValues[k * currentLayerSize + j];

                    _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        _previousLayerUpperBounds[i * previousLayerSize + k] *
                        weights._positiveValues[k * currentLayerSize + j];

                    _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        _previousLayerLowerBounds[i * previousLayerSize + k] *
                        weights._negativeValues[k * currentLayerSize + j];
                }
            }
        }

        /*
          Compute the biases for the new layer
        */
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _currentLayerLowerBias[j] = _biases[currentLayer][j];
            _currentLayerUpperBias[j] = _biases[currentLayer][j];

            // Add the weighted bias from the previous layer
            for ( unsigned k = 0; k < previousLayerSize; ++k )
            {
                double weight = weights._positiveValues[k * currentLayerSize + j] + weights._negativeValues[k * currentLayerSize + j];

                if ( weight > 0 )
                {
                    _currentLayerLowerBias[j] += _previousLayerLowerBias[k] * weight;
                    _currentLayerUpperBias[j] += _previousLayerUpperBias[k] * weight;
                }
                else
                {
                    _currentLayerLowerBias[j] += _previousLayerUpperBias[k] * weight;
                    _currentLayerUpperBias[j] += _previousLayerLowerBias[k] * weight;
                }
            }
        }

        log( "\nAfter matrix multiplication, newLB is:\n" );
        for ( unsigned i = 0; i < _inputLayerSize; ++i )
        {
            log( "\t" );
            for ( unsigned j = 0; j < _layerSizes[currentLayer]; ++j )
            {
                log( Stringf( "%.2lf ", _currentLayerLowerBounds[i*_layerSizes[currentLayer] + j] ) );
            }
            log( "\n" );
        }
        log( "\nnew UB is:\n" );
        for ( unsigned i = 0; i < _inputLayerSize; ++i )
        {
            log( "\t" );
            for ( unsigned j = 0; j < _layerSizes[currentLayer]; ++j )
            {
                log( Stringf( "%.2lf ", _currentLayerUpperBounds[i*_layerSizes[currentLayer] + j] ) );
            }
            log( "\n" );
        }

        // We now have the symbolic representation for the new layer. Next, we compute new lower
        // and upper bounds for it. For each of these bounds, we compute an upper bound and a lower
        // bound.
        //
        // newUB, newLB dimensions: inputLayerSize x layerSize
        //
        for ( unsigned i = 0; i < currentLayerSize; ++i )
        {
            double lbLb = 0;
            double lbUb = 0;
            double ubLb = 0;
            double ubUb = 0;

            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                double entry = _currentLayerLowerBounds[j * currentLayerSize + i];

                if ( entry >= 0 )
                {
                    lbLb += ( entry * _inputLowerBounds[j] );
                    lbUb += ( entry * _inputUpperBounds[j] );
                }
                else
                {
                    lbLb += ( entry * _inputUpperBounds[j] );
                    lbUb += ( entry * _inputLowerBounds[j] );
                }

                lbLb -= BOUND_ROUNDING_CONSTANT;
                lbUb += BOUND_ROUNDING_CONSTANT;

                entry = _currentLayerUpperBounds[j * currentLayerSize + i];

                if ( entry >= 0 )
                {
                    ubLb += ( entry * _inputLowerBounds[j] );
                    ubUb += ( entry * _inputUpperBounds[j] );
                }
                else
                {
                    ubLb += ( entry * _inputUpperBounds[j] );
                    ubUb += ( entry * _inputLowerBounds[j] );
                }

                ubLb -= BOUND_ROUNDING_CONSTANT;
                ubUb += BOUND_ROUNDING_CONSTANT;
            }

            // Add the network bias to all bounds
            lbLb += _currentLayerLowerBias[i];
            lbUb += _currentLayerLowerBias[i];
            ubLb += _currentLayerUpperBias[i];
            ubUb += _currentLayerUpperBias[i];

            log( Stringf( "Neuron %u: Computed concrete lb: %lf, ub: %lf\n", i, lbLb, ubUb ) );

            // Handle the ReLU activation. We know that:
            //   lbLb <= true LB <= lbUb
            //   ubLb <= true UB <= ubUb
            if ( currentLayer < _numberOfLayers - 1 )
            {
                // If the ReLU phase is not fixed yet, do the usual propagation:
                NodeIndex reluIndex( currentLayer, i );
                if ( !_nodeIndexToReluState.exists( reluIndex ) || _nodeIndexToReluState[reluIndex] == ReluConstraint::PHASE_NOT_FIXED )
                {

                    if ( ubUb <= 0 )
                    {
                        // lb <= ub <= 0
                        // The ReLU will zero this entry out
                        lbLb = 0;
                        lbUb = 0;
                        ubLb = 0;
                        ubUb = 0;

                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                        {
                            _currentLayerLowerBounds[j * currentLayerSize + i] = 0;
                            _currentLayerUpperBounds[j * currentLayerSize + i] = 0;
                        }
                        _currentLayerLowerBias[i] = 0;
                        _currentLayerUpperBias[i] = 0;
                    }
                    else if ( lbLb >= 0 )
                    {
                        // 0 <= lb <= ub
                        // The ReLU will not affect this entry

                        log( "SBT: eliminated nothing!\n" );
                    }
                    else
                    {
                        // lbLb <= 0 <= ubUb
                        // The ReLU might affect this entry, we need to figure out how

                        if ( ubLb < 0 )
                        {
                            // The upper bound range goes below 0, we we need to zero it out
                            for ( unsigned j = 0; j < _inputLayerSize; ++j )
                                _currentLayerUpperBounds[j * currentLayerSize + i] = 0;

                            // We keep the concrete maxiaml value of the upper bound as the bias for this layer
                            _currentLayerUpperBias[i] = ubUb;
                        }
                        else
                        {
                            log( "SBT: did not eliminate upper!\n" );
                        }

                        // The lower bound can be negative, so it is zeroed out also
                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                            _currentLayerLowerBounds[j * currentLayerSize + i] = 0;

                        _currentLayerLowerBias[i] = 0;
                        lbLb = 0;
                    }

                    log( Stringf( "\tAfter ReLU: concrete lb: %lf, ub: %lf\n", lbLb, ubUb ) );
                }
                else
                {
                    // The phase of this ReLU is fixed!
                    if ( _nodeIndexToReluState[reluIndex] == ReluConstraint::PHASE_ACTIVE )
                    {
                        // Active ReLU, bounds are propagated as is
                    }
                    else
                    {
                        // Inactive ReLU, returns zero
                        lbLb = 0;
                        lbUb = 0;
                        ubLb = 0;
                        ubUb = 0;

                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                        {
                            _currentLayerLowerBounds[j * currentLayerSize + i] = 0;
                            _currentLayerUpperBounds[j * currentLayerSize + i] = 0;
                        }
                        _currentLayerLowerBias[i] = 0;
                        _currentLayerUpperBias[i] = 0;
                    }

                    log( Stringf( "\tAfter phase-fixed ReLU: concrete lb: %lf, ub: %lf\n", lbLb, ubUb ) );
                }
            }

            // Store the bounds for this neuron
            _lowerBounds[currentLayer][i] = lbLb;
            _upperBounds[currentLayer][i] = ubUb;
        }

        log( "Dumping current layer upper bounds, before copy:\n" );
        for ( unsigned i = 0; i < _maxLayerSize * _inputLayerSize; ++i )
            log( Stringf( "%.5lf ", _currentLayerUpperBounds[i] ) );
        log( "\n\n" );

        // Prepare for next iteration
        memcpy( _previousLayerLowerBounds, _currentLayerLowerBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
        memcpy( _previousLayerUpperBounds, _currentLayerUpperBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
        memcpy( _previousLayerLowerBias, _currentLayerLowerBias, sizeof(double) * _maxLayerSize );
        memcpy( _previousLayerUpperBias, _currentLayerUpperBias, sizeof(double) * _maxLayerSize );
    }
}

double SymbolicBoundTightener::getLowerBound( unsigned layer, unsigned neuron ) const
{
    return _lowerBounds[layer][neuron];
}

double SymbolicBoundTightener::getUpperBound( unsigned layer, unsigned neuron ) const
{
    return _upperBounds[layer][neuron];
}

void SymbolicBoundTightener::log( const String &message )
{
    if ( GlobalConfiguration::SYMBOLIC_BOUND_TIGHTENER_LOGGING )
        printf( "SymbolicBoundTightener: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
