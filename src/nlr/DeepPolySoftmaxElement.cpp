/*********************                                                        */
/*! \file DeepPolySoftmaxElement.cpp
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

#include "DeepPolySoftmaxElement.h"
#include "FloatUtils.h"
#include "SoftmaxConstraint.h"

#include <string.h>

namespace NLR {

DeepPolySoftmaxElement::DeepPolySoftmaxElement( Layer *layer )
    : _workLb( NULL )
    , _workUb( NULL )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolySoftmaxElement::~DeepPolySoftmaxElement()
{
    freeMemoryIfNeeded();
}

void DeepPolySoftmaxElement::execute
( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();
    getConcreteBounds();

    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        log( Stringf( "Handling Neuron %u_%u...", _layerIndex, i ) );
        List<NeuronIndex> sources = _layer->getActivationSources( i );

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        Vector<double> sourceMids;
        for ( const auto &sourceIndex : sources )
        {
            DeepPolyElement *predecessor =
                deepPolyElementsBefore[sourceIndex._layer];
            double sourceLb = predecessor->getLowerBound
              ( sourceIndex._neuron ) - GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;
            sourceLbs.append(sourceLb);
            double sourceUb = predecessor->getUpperBound
              ( sourceIndex._neuron ) + GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;
            sourceUbs.append(sourceUb);
            sourceMids.append((sourceLb + sourceUb) / 2);
        }

        // Compute concrete bound

        double lb = L_Linear(sourceLbs, sourceUbs, i);
        double ub = U_Linear(sourceLbs, sourceUbs, i);
        if ( lb > _lb[i])
          _lb[i] = lb;
        if ( ub < _ub[i])
          _ub[i] = ub;
        log( Stringf( "Current bounds of neuron %u: [%f, %f]", i,
                      _lb[i], _ub[i] ) );

        // Compute symbolic bound
        _symbolicLowerBias[i] = L_LSE(sourceMids, sourceLbs, sourceUbs, i);
        _symbolicUpperBias[i] = U_LSE(sourceMids, i);

        for (unsigned j = 0; j < _size; ++j)
        {
          double dldj = dLdx(sourceMids, sourceLbs, sourceUbs,i, j);
          _symbolicLb[_size * j + i] = dldj;
          _symbolicLowerBias[i] -= dldj * sourceMids[j];

          double dudj = dUdx(sourceMids, i, j);
          _symbolicUb[_size * j + i] = dudj;
          _symbolicUpperBias[i] -= dudj * sourceMids[j];
        }
    }

    // Compute bounds with back-substitution
    computeBoundWithBackSubstitution( deepPolyElementsBefore );
    log( "Executing - done" );
}

void DeepPolySoftmaxElement::computeBoundWithBackSubstitution
( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Computing bounds with back substitution..." );

    // Start with the symbolic upper-/lower- bounds of this layer with
    // respect to its immediate predecessor.
    Map<unsigned, unsigned> predecessorIndices = getPredecessorIndices();
    ASSERT( predecessorIndices.size() == 1 );

    unsigned predecessorIndex = predecessorIndices.begin()->first;

    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessorIndex ) );
    DeepPolyElement *precedingElement =
        deepPolyElementsBefore[predecessorIndex];
    unsigned sourceLayerSize = precedingElement->getSize();

    ASSERT(sourceLayerSize == _size);

    memcpy( _work1SymbolicLb,
            _symbolicLb, _size * sourceLayerSize * sizeof(double) );
    memcpy( _work1SymbolicUb,
            _symbolicUb, _size * sourceLayerSize * sizeof(double) );

    memcpy( _workSymbolicLowerBias, _symbolicLowerBias, _size * sizeof(double) );
    memcpy( _workSymbolicUpperBias, _symbolicUpperBias, _size * sizeof(double) );

    DeepPolyElement *currentElement = precedingElement;
    concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                             _workSymbolicLowerBias,
                             _workSymbolicUpperBias,
                             currentElement, deepPolyElementsBefore );
    log( Stringf( "Computing symbolic bounds with respect to layer %u - done",
                  predecessorIndex ) );

    while ( currentElement->hasPredecessor() )
    {
        // We have the symbolic bounds in terms of the current abstract
        // element--currentElement, stored in _work1SymbolicLb,
        // _work1SymbolicUb, _workSymbolicLowerBias, _workSymbolicLowerBias,
        // now compute the symbolic bounds in terms of currentElement's
        // predecessor.
        predecessorIndices = currentElement->getPredecessorIndices();
        unsigned counter = 0;
        unsigned numPredecessors = predecessorIndices.size();
        ASSERT( numPredecessors > 0 );
        for ( const auto &pair : predecessorIndices )
        {
            predecessorIndex = pair.first;
            precedingElement = deepPolyElementsBefore[predecessorIndex];

            if ( counter < numPredecessors - 1 )
            {
                unsigned predecessorIndex = pair.first;
                log( Stringf( "Adding residual from layer %u...",
                              predecessorIndex ) );
                allocateMemoryForResidualsIfNeeded( predecessorIndex,
                                                    pair.second );
                // Do we need to add bias here?
                currentElement->symbolicBoundInTermsOfPredecessor
                    ( _work1SymbolicLb, _work1SymbolicUb, NULL, NULL,
                      _residualLb[predecessorIndex],
                      _residualUb[predecessorIndex],
                      _size, precedingElement );
                ++counter;
                log( Stringf( "Adding residual from layer %u - done", pair.first ) );
            }
        }

        std::fill_n( _work2SymbolicLb, _size * precedingElement->getSize(), 0 );
        std::fill_n( _work2SymbolicUb, _size * precedingElement->getSize(), 0 );
        currentElement->symbolicBoundInTermsOfPredecessor
            ( _work1SymbolicLb, _work1SymbolicUb, _workSymbolicLowerBias,
              _workSymbolicUpperBias, _work2SymbolicLb, _work2SymbolicUb,
              _size, precedingElement );

        // The symbolic lower-bound is
        // _work2SymbolicLb * precedingElement + residualLb1 * residualElement1 +
        // residualLb2 * residualElement2 + ...
        // If the precedingElement is a residual source layer, we can merge
        // in the residualWeights, and remove it from the residual source layers.
        if ( _residualLayerIndices.exists( predecessorIndex ) )
        {
            log( Stringf( "merge residual from layer %u...", predecessorIndex ) );
            // Add weights of this residual layer
            for ( unsigned i = 0; i < _size * precedingElement->getSize(); ++i )
            {
                _work2SymbolicLb[i] += _residualLb[predecessorIndex][i];
                _work2SymbolicUb[i] += _residualUb[predecessorIndex][i];
            }
            _residualLayerIndices.erase( predecessorIndex );
            std::fill_n( _residualLb[predecessorIndex],
                         _size * precedingElement->getSize(), 0 );
            std::fill_n( _residualUb[predecessorIndex],
                         _size * precedingElement->getSize(), 0 );
            log( Stringf( "merge residual from layer %u - done", predecessorIndex ) );
        }

        DEBUG({
                // Residual layers topologically after precedingElement should
                // have been merged already.
                for ( const auto &residualLayerIndex : _residualLayerIndices )
                {
                    ASSERT( residualLayerIndex < predecessorIndex );
                }
            });

        double *temp = _work1SymbolicLb;
        _work1SymbolicLb = _work2SymbolicLb;
        _work2SymbolicLb = temp;

        temp = _work1SymbolicUb;
        _work1SymbolicUb = _work2SymbolicUb;
        _work2SymbolicUb = temp;

        currentElement = precedingElement;
        concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                                 _workSymbolicLowerBias, _workSymbolicUpperBias,
                                 currentElement, deepPolyElementsBefore );
    }
    ASSERT( _residualLayerIndices.empty() );
    log( "Computing bounds with back substitution - done" );
}

void DeepPolySoftmaxElement::concretizeSymbolicBound
( const double *symbolicLb, const double*symbolicUb, double const
  *symbolicLowerBias, const double *symbolicUpperBias, DeepPolyElement
  *sourceElement, const Map<unsigned, DeepPolyElement *>
  &deepPolyElementsBefore )
{
    log( "Concretizing bound..." );
    std::fill_n( _workLb, _size, 0 );
    std::fill_n( _workUb, _size, 0 );

    concretizeSymbolicBoundForSourceLayer( symbolicLb, symbolicUb,
                                           symbolicLowerBias, symbolicUpperBias,
                                           sourceElement );

    for ( const auto &residualLayerIndex : _residualLayerIndices )
    {
        ASSERT( residualLayerIndex < sourceElement->getLayerIndex() );
        DeepPolyElement *residualElement =
            deepPolyElementsBefore[residualLayerIndex];
        concretizeSymbolicBoundForSourceLayer( _residualLb[residualLayerIndex],
                                               _residualUb[residualLayerIndex],
                                               NULL,
                                               NULL,
                                               residualElement );
    }
    for ( unsigned i = 0; i <_size; ++i )
    {
      log( Stringf( "Neuron%u previous LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
        if ( _lb[i] < _workLb[i] )
            _lb[i] = _workLb[i];
        if ( _ub[i] > _workUb[i] )
            _ub[i] = _workUb[i];
        log( Stringf( "Neuron%u working LB: %f, UB: %f", i, _workLb[i], _workUb[i] ) );
    }

    log( "Concretizing bound - done" );
}

void DeepPolySoftmaxElement::concretizeSymbolicBoundForSourceLayer
( const double *symbolicLb, const double*symbolicUb, const double
  *symbolicLowerBias, const double *symbolicUpperBias, DeepPolyElement
  *sourceElement )
{
    DEBUG({
            log( Stringf( "Source layer: %u", sourceElement->getLayerIndex() ) );
            String s = Stringf( "Symbolic lowerbounds w.r.t. layer %u: \n ", sourceElement->getLayerIndex() );
            for ( unsigned i = 0; i <_size; ++i )
            {
                for ( unsigned j = 0; j < sourceElement->getSize(); ++j )
                {
                    s += Stringf( "%f ", symbolicLb[j * _size + i] );
                }
                s += "\n";
            }
            s += "\n";
            if ( symbolicLowerBias )
            {
                s += Stringf( "Symbolic lower bias w.r.t. layer %u: \n ", sourceElement->getLayerIndex() );
                for ( unsigned i = 0; i <_size; ++i )
                {
                    s += Stringf( "%f ", symbolicLowerBias[i] );
                }
                s += "\n";
            }
            s += Stringf( "Symbolic upperbounds w.r.t. layer %u: \n ", sourceElement->getLayerIndex() );
            for ( unsigned i = 0; i <_size; ++i )
            {
                for ( unsigned j = 0; j < sourceElement->getSize(); ++j )
                {
                    s += Stringf( "%f ", symbolicUb[j * _size + i] );
                }
                s += "\n";
            }
            s += "\n";
            if ( symbolicUpperBias )
            {
                s += Stringf( "Symbolic upper bias w.r.t. layer %u: \n ", sourceElement->getLayerIndex() );
                for ( unsigned i = 0; i <_size; ++i )
                {
                    s += Stringf( "%f ", symbolicUpperBias[i] );
                }
                s += "\n";
            }
            log( s );
        });

    // Get concrete bounds
    for ( unsigned i = 0; i < sourceElement->getSize(); ++i )
    {
        double sourceLb = sourceElement->getLowerBoundFromLayer( i );
        double sourceUb = sourceElement->getUpperBoundFromLayer( i );

        log( Stringf( "Bounds of neuron%u_%u: [%f, %f]", sourceElement->
                      getLayerIndex(), i, sourceLb, sourceUb ) );

        for ( unsigned j = 0; j < _size; ++j )
        {
            // Compute lower bound
            double weight = symbolicLb[i * _size + j];
            if ( weight >= 0 )
            {
                _workLb[j] += ( weight * sourceLb );
            } else
            {
                _workLb[j] += ( weight * sourceUb );
            }

            // Compute upper bound
            weight = symbolicUb[i * _size + j];
            if ( weight >= 0 )
            {
                _workUb[j] += ( weight * sourceUb );
            } else
            {
                _workUb[j] += ( weight * sourceLb );
            }
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( symbolicLowerBias )
            _workLb[i] += symbolicLowerBias[i];
        if ( symbolicUpperBias )
            _workUb[i] += symbolicUpperBias[i];
    }
}


void DeepPolySoftmaxElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
  std::cout << "Should never reach here!" << std::endl;

    unsigned predecessorIndex = predecessor->getLayerIndex();
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessorIndex ) );
    unsigned predecessorSize = predecessor->getSize();

    double *weights = _layer->getWeights( predecessorIndex );
    double *biases = _layer->getBiases();

    // newSymbolicLb = weights * symbolicLb
    // newSymbolicUb = weights * symbolicUb
    matrixMultiplication( weights, symbolicLb,
                          symbolicLbInTermsOfPredecessor, predecessorSize,
                          _size, targetLayerSize );
    matrixMultiplication( weights, symbolicUb,
                          symbolicUbInTermsOfPredecessor, predecessorSize,
                          _size, targetLayerSize );

    // symbolicLowerBias = biases * symbolicLb
    // symbolicUpperBias = biases * symbolicUb
    if  ( symbolicLowerBias )
        matrixMultiplication( biases, symbolicLb, symbolicLowerBias, 1,
                              _size, targetLayerSize );
    if  ( symbolicUpperBias )
        matrixMultiplication( biases, symbolicUb, symbolicUpperBias, 1,
                              _size, targetLayerSize );
    log( Stringf( "Computing symbolic bounds with respect to layer %u - done",
                  predecessorIndex ) );
}

void DeepPolySoftmaxElement::allocateMemoryForResidualsIfNeeded
( unsigned residualLayerIndex, unsigned residualLayerSize )
{
    _residualLayerIndices.insert( residualLayerIndex );
    unsigned matrixSize = residualLayerSize * _size;
    if ( !_residualLb.exists( residualLayerIndex ) )
    {
        double *residualLb = new double[matrixSize];
        std::fill_n( residualLb, matrixSize, 0 );
        _residualLb[residualLayerIndex] = residualLb;
    }
    if ( !_residualUb.exists( residualLayerIndex ) )
    {
        double *residualUb = new double[residualLayerSize * _size];
        std::fill_n( residualUb, matrixSize, 0 );
        _residualUb[residualLayerIndex] = residualUb;
    }
}

void DeepPolySoftmaxElement::allocateMemory()
{
    freeMemoryIfNeeded();

    DeepPolyElement::allocateMemory();

    _workLb = new double[_size];
    _workUb = new double[_size];

    std::fill_n( _workLb, _size, FloatUtils::negativeInfinity() );
    std::fill_n( _workUb, _size, FloatUtils::infinity() );

    unsigned size = _size * _size;
    _symbolicLb = new double[size];
    _symbolicUb = new double[size];

    std::fill_n( _symbolicLb, size, 0 );
    std::fill_n( _symbolicUb, size, 0 );

    _symbolicLowerBias = new double[_size];
    _symbolicUpperBias = new double[_size];

    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );
}

void DeepPolySoftmaxElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
    if ( _workLb )
    {
        delete[] _workLb;
        _workLb = NULL;
    }
    if ( _workUb )
    {
        delete[] _workUb;
        _workUb = NULL;
    }
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
    for ( auto const &pair : _residualLb )
    {
        delete[] pair.second;
    }
    _residualLb.clear();
    for ( auto const &pair : _residualUb )
    {
        delete[] pair.second;
    }
    _residualUb.clear();
    _residualLayerIndices.clear();
}

void DeepPolySoftmaxElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolySoftmaxElement: %s\n", message.ascii() );
}

double DeepPolySoftmaxElement::L_LSE( const Vector<double> &input,
                                      const Vector<double> &inputLb,
                                      const Vector<double> &inputUb,
                                      unsigned i )
{
  double sum = 0;
  for (unsigned j = 0; j < input.size(); ++j) {
    double lj = inputLb[j];
    double uj = inputUb[j];
    double xj = input[j];

    sum += (uj - xj) / (uj - lj) * std::exp(lj) + (xj - lj)/(uj - lj) * std::exp(uj);
  }

  return std::exp(input[i]) / sum;
}

double DeepPolySoftmaxElement::dLdx( const Vector<double> &c,
                                     const Vector<double> &inputLb,
                                     const Vector<double> &inputUb,
                                     unsigned i, unsigned di)
{
  double val = 0;
  if (i == di)
    val += L_LSE(c, inputLb, inputUb, i);

  double ldi = inputLb[di];
  double udi = inputUb[di];

  double sum = 0;
  for (unsigned j = 0; j < c.size(); ++j) {
    double lj = inputLb[j];
    double uj = inputUb[j];
    double xj = c[j];

    sum += (uj - xj) / (uj - lj) * std::exp(lj) + (xj - lj)/(uj - lj) * std::exp(uj);
  }

  val -= std::exp(c[i]) / (sum * sum) * (std::exp(udi) - std::exp(ldi)) / (udi - ldi);

  return val;
}


double DeepPolySoftmaxElement::U_LSE( const Vector<double> &input, unsigned i )
{
  double li = _lb[i];
  double ui = _ub[i];

  Vector<double> inputTilda;
  SoftmaxConstraint::xTilda(input, input[i], inputTilda);

  return ((li * std::log(ui) - ui * std::log(li)) / (std::log(ui) - std::log(li)) -
          (ui - li) / (std::log(ui) - std::log(li)) * SoftmaxConstraint::LSE(inputTilda));
}

double DeepPolySoftmaxElement::dUdx( const Vector<double> &c,
                                     unsigned i, unsigned di)
{
  double li = _lb[i];
  double ui = _ub[i];

  double val = -(ui - li) / (std::log(ui) - std::log(li));

  double val2 = std::exp(c[i]) / SoftmaxConstraint::SE(c);
  if (i == di)
    val2 -= 1;

  return val * val2;
}

double DeepPolySoftmaxElement::L_Linear( const Vector<double> &inputLb,
                                         const Vector<double> &inputUb,
                                         unsigned i )
{
  Vector<double> uTilda;
  SoftmaxConstraint::xTilda(inputUb, inputLb[i], uTilda);
  uTilda[i] = 0;
  return 1 / SoftmaxConstraint::SE(uTilda);
}

double DeepPolySoftmaxElement::U_Linear( const Vector<double> &inputLb,
                                         const Vector<double> &inputUb,
                                         unsigned i )
{
  Vector<double> lTilda;
  SoftmaxConstraint::xTilda(inputLb, inputUb[i], lTilda);
  lTilda[i] = 0;
  return 1 / SoftmaxConstraint::SE(lTilda);


}

} // namespace NLR
