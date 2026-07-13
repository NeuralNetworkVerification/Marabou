/*********************                                                        */
/*! \file NetworkLevelReasoner.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Ido Shmuel
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "NetworkLevelReasoner.h"

#include "AbsoluteValueConstraint.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "IterativePropagator.h"
#include "MILPFormulator.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MatrixMultiplication.h"
#include "MaxConstraint.h"
#include "NLRError.h"
#include "Options.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "SignConstraint.h"

#include <cstring>

#define NLR_LOG( x, ... ) LOG( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING, "NLR: %s\n", x )

namespace NLR {

NetworkLevelReasoner::NetworkLevelReasoner()
    : _tableau( NULL )
    , _deepPolyAnalysis( nullptr )
{
}

NetworkLevelReasoner::~NetworkLevelReasoner()
{
    freeMemoryIfNeeded();
}

bool NetworkLevelReasoner::functionTypeSupported( PiecewiseLinearFunctionType type )
{
    if ( type == PiecewiseLinearFunctionType::RELU )
        return true;

    if ( type == PiecewiseLinearFunctionType::ABSOLUTE_VALUE )
        return true;

    if ( type == PiecewiseLinearFunctionType::SIGN )
        return true;

    return false;
}

void NetworkLevelReasoner::addLayer( unsigned layerIndex, Layer::Type type, unsigned layerSize )
{
    Layer *layer = new Layer( layerIndex, type, layerSize, this );
    _layerIndexToLayer[layerIndex] = layer;
}

void NetworkLevelReasoner::addLayerDependency( unsigned sourceLayer, unsigned targetLayer )
{
    _layerIndexToLayer[targetLayer]->addSourceLayer( sourceLayer,
                                                     _layerIndexToLayer[sourceLayer]->getSize() );
}

void NetworkLevelReasoner::removeLayerDependency( unsigned sourceLayer, unsigned targetLayer )
{
    _layerIndexToLayer[targetLayer]->removeSourceLayer( sourceLayer );
}

void NetworkLevelReasoner::computeSuccessorLayers()
{
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
    {
        for ( const auto &pair : _layerIndexToLayer[i]->getSourceLayers() )
        {
            _layerIndexToLayer[pair.first]->addSuccessorLayer( i );
        }
    }
}

void NetworkLevelReasoner::setWeight( unsigned sourceLayer,
                                      unsigned sourceNeuron,
                                      unsigned targetLayer,
                                      unsigned targetNeuron,
                                      double weight )
{
    _layerIndexToLayer[targetLayer]->setWeight( sourceLayer, sourceNeuron, targetNeuron, weight );
}

void NetworkLevelReasoner::setBias( unsigned layer, unsigned neuron, double bias )
{
    _layerIndexToLayer[layer]->setBias( neuron, bias );
}

void NetworkLevelReasoner::addActivationSource( unsigned sourceLayer,
                                                unsigned sourceNeuron,
                                                unsigned targetLayer,
                                                unsigned targetNeuron )
{
    _layerIndexToLayer[targetLayer]->addActivationSource( sourceLayer, sourceNeuron, targetNeuron );
}

const Layer *NetworkLevelReasoner::getLayer( unsigned index ) const
{
    return _layerIndexToLayer[index];
}

Layer *NetworkLevelReasoner::getLayer( unsigned index )
{
    return _layerIndexToLayer[index];
}

void NetworkLevelReasoner::evaluate( double *input, double *output )
{
    _layerIndexToLayer[0]->setAssignment( input );
    for ( unsigned i = 1; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeAssignment();

    const Layer *outputLayer = _layerIndexToLayer[_layerIndexToLayer.size() - 1];
    memcpy( output, outputLayer->getAssignment(), sizeof( double ) * outputLayer->getSize() );
}

void NetworkLevelReasoner::concretizeInputAssignment( Map<unsigned, double> &assignment )
{
    Layer *inputLayer = _layerIndexToLayer[0];
    ASSERT( inputLayer->getLayerType() == Layer::INPUT );

    unsigned inputLayerSize = inputLayer->getSize();
    ASSERT( inputLayerSize > 0 );

    double *input = new double[inputLayerSize];

    // First obtain the input assignment from the _tableau
    for ( unsigned index = 0; index < inputLayerSize; ++index )
    {
        if ( !inputLayer->neuronEliminated( index ) )
        {
            unsigned variable = inputLayer->neuronToVariable( index );
            double value = _tableau->getValue( variable );
            input[index] = value;
            assignment[variable] = value;
        }
        else
            input[index] = inputLayer->getEliminatedNeuronValue( index );
    }

    _layerIndexToLayer[0]->setAssignment( input );

    // Evaluate layers iteratively and store the results in "assignment"
    for ( unsigned i = 1; i < _layerIndexToLayer.size(); ++i )
    {
        Layer *currentLayer = _layerIndexToLayer[i];
        currentLayer->computeAssignment();
        for ( unsigned index = 0; index < currentLayer->getSize(); ++index )
        {
            if ( !currentLayer->neuronEliminated( index ) )
                assignment[currentLayer->neuronToVariable( index )] =
                    currentLayer->getAssignment( index );
        }
    }

    delete[] input;
}

void NetworkLevelReasoner::simulate( Vector<Vector<double>> *input )
{
    _layerIndexToLayer[0]->setSimulations( input );
    for ( unsigned i = 1; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeSimulations();
}

void NetworkLevelReasoner::setNeuronVariable( NeuronIndex index, unsigned variable )
{
    _layerIndexToLayer[index._layer]->setNeuronVariable( index._neuron, variable );
}

void NetworkLevelReasoner::receiveTighterBound( Tightening tightening )
{
    _boundTightenings.append( tightening );
}

void NetworkLevelReasoner::getConstraintTightenings( List<Tightening> &tightenings )
{
    tightenings = _boundTightenings;
    _boundTightenings.clear();
}

void NetworkLevelReasoner::clearConstraintTightenings()
{
    _boundTightenings.clear();
}

void NetworkLevelReasoner::receivePolygonalTightening( PolygonalTightening &polygonalTightening )
{
    _polygonalBoundTightenings.append( polygonalTightening );
}

void NetworkLevelReasoner::getPolygonalTightenings(
    List<PolygonalTightening> &polygonalTightenings )
{
    polygonalTightenings = _polygonalBoundTightenings;
    _polygonalBoundTightenings.clear();
}

void NetworkLevelReasoner::clearPolygonalTightenings()
{
    _polygonalBoundTightenings.clear();
}

void NetworkLevelReasoner::receiveInfeasibleBranches(
    Map<NeuronIndex, unsigned> &neuronToBranchIndex )
{
    _infeasibleBranches.append( neuronToBranchIndex );
}

void NetworkLevelReasoner::getInfeasibleBranches(
    List<Map<NeuronIndex, unsigned>> &infeasibleBranches )
{
    infeasibleBranches = _infeasibleBranches;
    _infeasibleBranches.clear();
}

void NetworkLevelReasoner::clearInfeasibleBranches()
{
    _infeasibleBranches.clear();
}

void NetworkLevelReasoner::symbolicBoundPropagation()
{
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeSymbolicBounds();
}

void NetworkLevelReasoner::parameterisedSymbolicBoundPropagation( const Vector<double> &coeffs )
{
    Map<unsigned, Vector<double>> layerIndicesToParameters = getParametersForLayers( coeffs );
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
    {
        const Vector<double> &currentLayerCoeffs = layerIndicesToParameters[i];
        _layerIndexToLayer[i]->computeParameterisedSymbolicBounds( currentLayerCoeffs, true );
    }
}

void NetworkLevelReasoner::deepPolyPropagation()
{
    if ( _deepPolyAnalysis == nullptr )
        _deepPolyAnalysis = std::unique_ptr<DeepPolyAnalysis>( new DeepPolyAnalysis( this ) );
    _deepPolyAnalysis->run();
}

void NetworkLevelReasoner::parameterisedDeepPoly( bool storeSymbolicBounds,
                                                  const Vector<double> &coeffs )
{
    if ( !storeSymbolicBounds )
    {
        bool useParameterisedSBT = coeffs.size() > 0;
        Map<unsigned, Vector<double>> layerIndicesToParameters =
            Map<unsigned, Vector<double>>( {} );
        if ( useParameterisedSBT )
        {
            layerIndicesToParameters = getParametersForLayers( coeffs );
        }

        _deepPolyAnalysis =
            std::unique_ptr<DeepPolyAnalysis>( new DeepPolyAnalysis( this,
                                                                     storeSymbolicBounds,
                                                                     storeSymbolicBounds,
                                                                     useParameterisedSBT,
                                                                     &layerIndicesToParameters ) );

        // Clear deepPolyAnalysis pointer after running.
        _deepPolyAnalysis->run();
        _deepPolyAnalysis = nullptr;
    }
    else
    {
        // Clear the previous symbolic bound maps.
        _outputSymbolicLb.clear();
        _outputSymbolicUb.clear();
        _outputSymbolicLowerBias.clear();
        _outputSymbolicUpperBias.clear();

        _predecessorSymbolicLb.clear();
        _predecessorSymbolicUb.clear();
        _predecessorSymbolicLowerBias.clear();
        _predecessorSymbolicUpperBias.clear();

        // Temporarily add weighted sum layer to the NLR of the same size of the output layer.
        Layer *outputLayer = _layerIndexToLayer[getNumberOfLayers() - 1];
        unsigned outputLayerIndex = outputLayer->getLayerIndex();
        unsigned outputLayerSize = outputLayer->getSize();
        unsigned newLayerIndex = outputLayerIndex + 1;

        addLayer( newLayerIndex, Layer::WEIGHTED_SUM, outputLayerSize );
        addLayerDependency( outputLayerIndex, newLayerIndex );
        Layer *newLayer = _layerIndexToLayer[newLayerIndex];

        for ( unsigned i = 0; i < outputLayerSize; ++i )
        {
            setWeight( outputLayerIndex, i, newLayerIndex, i, 1 );
            newLayer->setLb( i, FloatUtils::infinity() );
            newLayer->setUb( i, FloatUtils::negativeInfinity() );
        }

        // Initialize maps with zero vectors.
        for ( const auto &pair : _layerIndexToLayer )
        {
            unsigned layerIndex = pair.first;
            Layer *layer = pair.second;
            unsigned size = layer->getSize();
            Layer::Type layerType = layer->getLayerType();

            _outputSymbolicLb[layerIndex] = Vector<double>( outputLayerSize * size, 0 );
            _outputSymbolicUb[layerIndex] = Vector<double>( outputLayerSize * size, 0 );
            _outputSymbolicLowerBias[layerIndex] = Vector<double>( outputLayerSize, 0 );
            _outputSymbolicUpperBias[layerIndex] = Vector<double>( outputLayerSize, 0 );

            if ( layerType != Layer::WEIGHTED_SUM && layerType != Layer::INPUT )
            {
                unsigned maxSourceSize = 0;
                for ( unsigned i = 0; i < size; ++i )
                {
                    unsigned sourceSize = layer->getActivationSources( i ).size();
                    maxSourceSize = sourceSize > maxSourceSize ? sourceSize : maxSourceSize;
                }
                _predecessorSymbolicLb[layerIndex] = Vector<double>( size * maxSourceSize, 0 );
                _predecessorSymbolicUb[layerIndex] = Vector<double>( size * maxSourceSize, 0 );
                _predecessorSymbolicLowerBias[layerIndex] = Vector<double>( size, 0 );
                _predecessorSymbolicUpperBias[layerIndex] = Vector<double>( size, 0 );
            }
        }

        // Populate symbolic bounds maps via DeepPoly.
        bool useParameterisedSBT = coeffs.size() > 0;
        Map<unsigned, Vector<double>> layerIndicesToParameters =
            Map<unsigned, Vector<double>>( {} );
        if ( useParameterisedSBT )
        {
            layerIndicesToParameters = getParametersForLayers( coeffs );
        }

        _deepPolyAnalysis = std::unique_ptr<DeepPolyAnalysis>(
            new DeepPolyAnalysis( this,
                                  storeSymbolicBounds,
                                  storeSymbolicBounds,
                                  useParameterisedSBT,
                                  &layerIndicesToParameters,
                                  &_outputSymbolicLb,
                                  &_outputSymbolicUb,
                                  &_outputSymbolicLowerBias,
                                  &_outputSymbolicUpperBias,
                                  &_predecessorSymbolicLb,
                                  &_predecessorSymbolicUb,
                                  &_predecessorSymbolicLowerBias,
                                  &_predecessorSymbolicUpperBias ) );

        // Clear deepPolyAnalysis pointer after running.
        _deepPolyAnalysis->run();
        _deepPolyAnalysis = nullptr;

        // Remove new weighted sum layer.
        removeLayerDependency( outputLayerIndex, newLayerIndex );
        _layerIndexToLayer.erase( newLayerIndex );
        if ( newLayer )
        {
            delete newLayer;
            newLayer = NULL;
        }
    }
}

void NetworkLevelReasoner::lpRelaxationPropagation()
{
    LPFormulator lpFormulator( this );
    lpFormulator.setCutoff( 0 );

    if ( Options::get()->getMILPSolverBoundTighteningType() ==
             MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_ONCE ||
         Options::get()->getMILPSolverBoundTighteningType() ==
             MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE )
        lpFormulator.optimizeBoundsWithLpRelaxation( _layerIndexToLayer, true );
    else if ( Options::get()->getMILPSolverBoundTighteningType() ==
              MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_PREIMAGE_APPROX )
    {
        const Vector<double> optimalCoeffs = OptimalParameterisedSymbolicBoundTightening();
        Map<unsigned, Vector<double>> layerIndicesToParameters =
            getParametersForLayers( optimalCoeffs );
        lpFormulator.optimizeBoundsWithLpRelaxation(
            _layerIndexToLayer, false, layerIndicesToParameters );
        lpFormulator.optimizeBoundsWithLpRelaxation(
            _layerIndexToLayer, true, layerIndicesToParameters );
    }
    else if ( Options::get()->getMILPSolverBoundTighteningType() ==
              MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_PMNR )
    {
        const Vector<double> optimalCoeffs = OptimalParameterisedSymbolicBoundTightening();
        Map<unsigned, Vector<double>> layerIndicesToParameters =
            getParametersForLayers( optimalCoeffs );
        const Vector<PolygonalTightening> &polygonalTightenings =
            OptimizeParameterisedPolygonalTightening();
        lpFormulator.optimizeBoundsWithLpRelaxation(
            _layerIndexToLayer, false, layerIndicesToParameters, polygonalTightenings );
        lpFormulator.optimizeBoundsWithLpRelaxation(
            _layerIndexToLayer, true, layerIndicesToParameters, polygonalTightenings );
    }
    else if ( Options::get()->getMILPSolverBoundTighteningType() ==
              MILPSolverBoundTighteningType::LP_RELAXATION )
        lpFormulator.optimizeBoundsWithLpRelaxation( _layerIndexToLayer );
    else if ( Options::get()->getMILPSolverBoundTighteningType() ==
              MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL )
        lpFormulator.optimizeBoundsWithIncrementalLpRelaxation( _layerIndexToLayer );
}

void NetworkLevelReasoner::LPTighteningForOneLayer( unsigned targetIndex )
{
    LPFormulator lpFormulator( this );
    lpFormulator.setCutoff( 0 );

    if ( Options::get()->getMILPSolverBoundTighteningType() ==
         MILPSolverBoundTighteningType::LP_RELAXATION )
        lpFormulator.optimizeBoundsOfOneLayerWithLpRelaxation( _layerIndexToLayer, targetIndex );

    // TODO: implement for LP_RELAXATION_INCREMENTAL
}

void NetworkLevelReasoner::MILPPropagation()
{
    MILPFormulator milpFormulator( this );
    milpFormulator.setCutoff( 0 );

    if ( Options::get()->getMILPSolverBoundTighteningType() ==
         MILPSolverBoundTighteningType::MILP_ENCODING )
        milpFormulator.optimizeBoundsWithMILPEncoding( _layerIndexToLayer );
    else if ( Options::get()->getMILPSolverBoundTighteningType() ==
              MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL )
        milpFormulator.optimizeBoundsWithIncrementalMILPEncoding( _layerIndexToLayer );
}

void NetworkLevelReasoner::MILPTighteningForOneLayer( unsigned targetIndex )
{
    MILPFormulator milpFormulator( this );
    milpFormulator.setCutoff( 0 );

    if ( Options::get()->getMILPSolverBoundTighteningType() ==
         MILPSolverBoundTighteningType::MILP_ENCODING )
        milpFormulator.optimizeBoundsOfOneLayerWithMILPEncoding( _layerIndexToLayer, targetIndex );

    // TODO: implement for MILP_ENCODING_INCREMENTAL
}

void NetworkLevelReasoner::iterativePropagation()
{
    IterativePropagator iterativePropagator( this );
    iterativePropagator.setCutoff( 0 );
    iterativePropagator.optimizeBoundsWithIterativePropagation( _layerIndexToLayer );
}

void NetworkLevelReasoner::intervalArithmeticBoundPropagation()
{
    for ( unsigned i = 1; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeIntervalArithmeticBounds();
}

void NetworkLevelReasoner::freeMemoryIfNeeded()
{
    for ( const auto &layer : _layerIndexToLayer )
        delete layer.second;
    _layerIndexToLayer.clear();
}

void NetworkLevelReasoner::storeIntoOther( NetworkLevelReasoner &other ) const
{
    other.freeMemoryIfNeeded();

    for ( const auto &layer : _layerIndexToLayer )
    {
        const Layer *thisLayer = layer.second;
        Layer *newLayer = new Layer( thisLayer );
        newLayer->setLayerOwner( &other );
        other._layerIndexToLayer[newLayer->getLayerIndex()] = newLayer;
    }

    // Other has fresh copies of the PLCs, so its topological order
    // shouldn't contain any stale data
    other._constraintsInTopologicalOrder.clear();
}

void NetworkLevelReasoner::updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                                  const Map<unsigned, unsigned> &mergedVariables )
{
    for ( auto &layer : _layerIndexToLayer )
        layer.second->updateVariableIndices( oldIndexToNewIndex, mergedVariables );
}

void NetworkLevelReasoner::obtainCurrentBounds( const Query &inputQuery )
{
    for ( const auto &layer : _layerIndexToLayer )
        layer.second->obtainCurrentBounds( inputQuery );
}

void NetworkLevelReasoner::obtainCurrentBounds()
{
    ASSERT( _tableau );
    for ( const auto &layer : _layerIndexToLayer )
        layer.second->obtainCurrentBounds();
}

void NetworkLevelReasoner::setTableau( const ITableau *tableau )
{
    _tableau = tableau;
}

const ITableau *NetworkLevelReasoner::getTableau() const
{
    return _tableau;
}

void NetworkLevelReasoner::eliminateVariable( unsigned variable, double value )
{
    for ( auto &layer : _layerIndexToLayer )
        layer.second->eliminateVariable( variable, value );
}

void NetworkLevelReasoner::dumpTopology( bool dumpLayerDetails ) const
{
    printf( "Number of layers: %u. Sizes:\n", _layerIndexToLayer.size() );
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
    {
        printf( "\tLayer %u: %u \t[%s]",
                i,
                _layerIndexToLayer[i]->getSize(),
                Layer::typeToString( _layerIndexToLayer[i]->getLayerType() ).ascii() );
        printf( "\tSource layers:" );
        for ( const auto &sourceLayer : _layerIndexToLayer[i]->getSourceLayers() )
            printf( " %u", sourceLayer.first );
        printf( "\n" );
    }
    if ( dumpLayerDetails )
        for ( const auto &layer : _layerIndexToLayer )
            layer.second->dump();
}

unsigned NetworkLevelReasoner::getNumberOfLayers() const
{
    return _layerIndexToLayer.size();
}

List<PiecewiseLinearConstraint *> NetworkLevelReasoner::getConstraintsInTopologicalOrder()
{
    return _constraintsInTopologicalOrder;
}

void NetworkLevelReasoner::addConstraintInTopologicalOrder( PiecewiseLinearConstraint *constraint )
{
    _constraintsInTopologicalOrder.append( constraint );
}

void NetworkLevelReasoner::removeConstraintFromTopologicalOrder(
    PiecewiseLinearConstraint *constraint )
{
    if ( _constraintsInTopologicalOrder.exists( constraint ) )
        _constraintsInTopologicalOrder.erase( constraint );
}

void NetworkLevelReasoner::encodeAffineLayers( Query &inputQuery )
{
    for ( const auto &pair : _layerIndexToLayer )
        if ( pair.second->getLayerType() == Layer::WEIGHTED_SUM )
            generateQueryForWeightedSumLayer( inputQuery, pair.second );
}

void NetworkLevelReasoner::generateQuery( Query &result )
{
    // Number of variables
    unsigned numberOfVariables = 0;
    for ( const auto &it : _layerIndexToLayer )
    {
        unsigned maxVariable = it.second->getMaxVariable();
        if ( maxVariable > numberOfVariables )
            numberOfVariables = maxVariable;
    }
    ++numberOfVariables;
    result.setNumberOfVariables( numberOfVariables );

    // Handle the various layers
    for ( const auto &it : _layerIndexToLayer )
        generateQueryForLayer( result, *it.second );

    // Mark the input variables
    const Layer *inputLayer = _layerIndexToLayer[0];
    for ( unsigned i = 0; i < inputLayer->getSize(); ++i )
        result.markInputVariable( inputLayer->neuronToVariable( i ), i );

    // Mark the output variables
    const Layer *outputLayer = _layerIndexToLayer[_layerIndexToLayer.size() - 1];
    for ( unsigned i = 0; i < outputLayer->getSize(); ++i )
        result.markOutputVariable( outputLayer->neuronToVariable( i ), i );

    // Store any known bounds of all layers
    for ( const auto &layerPair : _layerIndexToLayer )
    {
        const Layer *layer = layerPair.second;
        for ( unsigned i = 0; i < layer->getSize(); ++i )
        {
            unsigned variable = layer->neuronToVariable( i );
            result.setLowerBound( variable, layer->getLb( i ) );
            result.setUpperBound( variable, layer->getUb( i ) );
        }
    }
}

void NetworkLevelReasoner::reindexNeurons()
{
    unsigned index = 0;
    for ( auto &it : _layerIndexToLayer )
    {
        for ( unsigned i = 0; i < it.second->getSize(); ++i )
        {
            it.second->setNeuronVariable( i, index );
            ++index;
        }
    }
}

void NetworkLevelReasoner::generateQueryForLayer( Query &inputQuery, const Layer &layer )
{
    switch ( layer.getLayerType() )
    {
    case Layer::INPUT:
        break;

    case Layer::WEIGHTED_SUM:
        generateQueryForWeightedSumLayer( inputQuery, layer );
        break;

    case Layer::RELU:
        generateQueryForReluLayer( inputQuery, layer );
        break;

    case Layer::SIGMOID:
        generateQueryForSigmoidLayer( inputQuery, layer );
        break;

    case Layer::SIGN:
        generateQueryForSignLayer( inputQuery, layer );
        break;

    case Layer::ABSOLUTE_VALUE:
        generateQueryForAbsoluteValueLayer( inputQuery, layer );
        break;

    case Layer::MAX:
        generateQueryForMaxLayer( inputQuery, layer );
        break;

    default:
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED,
                        Stringf( "Layer %u not yet supported", layer.getLayerType() ).ascii() );
        break;
    }
}

void NetworkLevelReasoner::generateQueryForReluLayer( Query &inputQuery, const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        NeuronIndex sourceIndex = *layer.getActivationSources( i ).begin();
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        ReluConstraint *relu = new ReluConstraint(
            sourceLayer->neuronToVariable( sourceIndex._neuron ), layer.neuronToVariable( i ) );
        inputQuery.addPiecewiseLinearConstraint( relu );
    }
}

void NetworkLevelReasoner::generateQueryForSigmoidLayer( Query &inputQuery, const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        NeuronIndex sourceIndex = *layer.getActivationSources( i ).begin();
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        SigmoidConstraint *sigmoid = new SigmoidConstraint(
            sourceLayer->neuronToVariable( sourceIndex._neuron ), layer.neuronToVariable( i ) );
        inputQuery.addNonlinearConstraint( sigmoid );
    }
}

void NetworkLevelReasoner::generateQueryForSignLayer( Query &inputQuery, const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        NeuronIndex sourceIndex = *layer.getActivationSources( i ).begin();
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        SignConstraint *sign = new SignConstraint(
            sourceLayer->neuronToVariable( sourceIndex._neuron ), layer.neuronToVariable( i ) );
        inputQuery.addPiecewiseLinearConstraint( sign );
    }
}

void NetworkLevelReasoner::generateQueryForAbsoluteValueLayer( Query &inputQuery,
                                                               const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        NeuronIndex sourceIndex = *layer.getActivationSources( i ).begin();
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        AbsoluteValueConstraint *absoluteValue = new AbsoluteValueConstraint(
            sourceLayer->neuronToVariable( sourceIndex._neuron ), layer.neuronToVariable( i ) );
        inputQuery.addPiecewiseLinearConstraint( absoluteValue );
    }
}

void NetworkLevelReasoner::generateQueryForMaxLayer( Query &inputQuery, const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        Set<unsigned> elements;
        for ( const auto &source : layer.getActivationSources( i ) )
        {
            const Layer *sourceLayer = _layerIndexToLayer[source._layer];
            elements.insert( sourceLayer->neuronToVariable( source._neuron ) );
        }

        MaxConstraint *max = new MaxConstraint( layer.neuronToVariable( i ), elements );
        inputQuery.addPiecewiseLinearConstraint( max );
    }
}

void NetworkLevelReasoner::generateQueryForWeightedSumLayer( Query &inputQuery, const Layer &layer )
{
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        Equation eq;
        eq.setScalar( -layer.getBias( i ) );
        eq.addAddend( -1, layer.neuronToVariable( i ) );

        for ( const auto &it : layer.getSourceLayers() )
        {
            const Layer *sourceLayer = _layerIndexToLayer[it.first];

            for ( unsigned j = 0; j < sourceLayer->getSize(); ++j )
            {
                double coefficient = layer.getWeight( sourceLayer->getLayerIndex(), j, i );
                if ( !FloatUtils::isZero( coefficient ) )
                    eq.addAddend( coefficient, sourceLayer->neuronToVariable( j ) );
            }
        }
        inputQuery.addEquation( eq );
    }
}

void NetworkLevelReasoner::generateLinearExpressionForWeightedSumLayer(
    Map<unsigned, LinearExpression> &variableToExpression,
    const Layer &layer )
{
    ASSERT( layer.getLayerType() == Layer::WEIGHTED_SUM );
    for ( unsigned i = 0; i < layer.getSize(); ++i )
    {
        LinearExpression exp;
        exp._constant = layer.getBias( i );
        for ( const auto &it : layer.getSourceLayers() )
        {
            const Layer *sourceLayer = _layerIndexToLayer[it.first];
            for ( unsigned j = 0; j < sourceLayer->getSize(); ++j )
            {
                double coefficient = layer.getWeight( sourceLayer->getLayerIndex(), j, i );
                if ( !FloatUtils::isZero( coefficient ) )
                {
                    unsigned var = sourceLayer->neuronToVariable( j );
                    if ( exp._addends.exists( var ) )
                        exp._addends[var] += coefficient;
                    else
                        exp._addends[var] = coefficient;
                }
            }
        }
        variableToExpression[layer.neuronToVariable( i )] = exp;
    }
}

/*
    Initialize and fill ReLU Constraint to previous bias map
    for BaBSR Heuristic
*/
void NetworkLevelReasoner::initializePreviousBiasMap()
{
    // Clear the previous bias map
    _previousBiases.clear();

    // Track accumulated ReLU neurons across layers
    unsigned accumulatedNeurons = 0;

    // Iterate through layers to find ReLU layers and their sources
    for ( const auto &layerPair : _layerIndexToLayer )
    {
        const NLR::Layer *layer = layerPair.second;

        if ( layer->getLayerType() == Layer::RELU )
        {
            // Get source layer info
            const auto &sourceLayers = layer->getSourceLayers();
            unsigned sourceLayerIndex = sourceLayers.begin()->first;
            const NLR::Layer *sourceLayer = getLayer( sourceLayerIndex );

            // Match ReLU constraints to their source layer biases
            unsigned layerSize = layer->getSize();

            // Iterate through constraints
            auto constraintIterator = _constraintsInTopologicalOrder.begin();
            for ( unsigned currentIndex = 0;
                  currentIndex < accumulatedNeurons &&
                  constraintIterator != _constraintsInTopologicalOrder.end();
                  ++currentIndex, ++constraintIterator )
            {
            }

            // Now at correct starting position
            for ( unsigned i = 0;
                  i < layerSize && constraintIterator != _constraintsInTopologicalOrder.end();
                  ++i, ++constraintIterator )
            {
                if ( auto reluConstraint =
                         dynamic_cast<const ReluConstraint *>( *constraintIterator ) )
                {
                    // Store bias in map
                    _previousBiases[reluConstraint] = sourceLayer->getBias( i );
                }
            }

            accumulatedNeurons += layerSize;
        }
    }
}

/*
    Get previous layer bias of a ReLU neuron
    for BaBSR Heuristic
*/
double NetworkLevelReasoner::getPreviousBias( const ReluConstraint *reluConstraint ) const
{
    // Initialize map if empty
    if ( _previousBiases.empty() )
    {
        const_cast<NetworkLevelReasoner *>( this )->initializePreviousBiasMap();
    }

    // Look up pre-computed bias
    if ( !_previousBiases.exists( reluConstraint ) )
    {
        throw NLRError( NLRError::RELU_NOT_FOUND, "ReluConstraint not found in bias map." );
    }

    return _previousBiases[reluConstraint];
}

Vector<double> NetworkLevelReasoner::getOutputSymbolicLb( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _outputSymbolicLb.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_outputSymbolicLb.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in output layer symbolic bounds map." );
    }

    return _outputSymbolicLb[layerIndex];
}

Vector<double> NetworkLevelReasoner::getOutputSymbolicUb( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _outputSymbolicUb.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_outputSymbolicUb.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in output layer symbolic bounds map." );
    }

    return _outputSymbolicUb[layerIndex];
}

Vector<double> NetworkLevelReasoner::getOutputSymbolicLowerBias( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _outputSymbolicLowerBias.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_outputSymbolicLowerBias.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in output layer symbolic bounds map." );
    }

    return _outputSymbolicLowerBias[layerIndex];
}

Vector<double> NetworkLevelReasoner::getOutputSymbolicUpperBias( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _outputSymbolicUpperBias.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_outputSymbolicUpperBias.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in output layer symbolic bounds map." );
    }

    return _outputSymbolicUpperBias[layerIndex];
}

Vector<double> NetworkLevelReasoner::getPredecessorSymbolicLb( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _predecessorSymbolicLb.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_predecessorSymbolicLb.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in predecessor layer symbolic bounds map." );
    }

    return _predecessorSymbolicLb[layerIndex];
}

Vector<double> NetworkLevelReasoner::getPredecessorSymbolicUb( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _predecessorSymbolicUb.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_predecessorSymbolicUb.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in predecessor layer symbolic bounds map." );
    }

    return _predecessorSymbolicUb[layerIndex];
}

Vector<double> NetworkLevelReasoner::getPredecessorSymbolicLowerBias( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _predecessorSymbolicLowerBias.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_predecessorSymbolicLowerBias.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in predecessor layer symbolic bounds map." );
    }

    return _predecessorSymbolicLowerBias[layerIndex];
}

Vector<double> NetworkLevelReasoner::getPredecessorSymbolicUpperBias( unsigned layerIndex )
{
    // Initialize map if empty.
    if ( _predecessorSymbolicUpperBias.empty() )
    {
        parameterisedDeepPoly( true );
    }

    if ( !_predecessorSymbolicUpperBias.exists( layerIndex ) )
    {
        throw NLRError( NLRError::LAYER_NOT_FOUND,
                        "Layer not found in predecessor layer symbolic bounds map." );
    }

    return _predecessorSymbolicUpperBias[layerIndex];
}

double NetworkLevelReasoner::getPMNRScore( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToPMNRScores.empty() )
    {
        initializePMNRScoreMap();
    }

    if ( !_neuronToPMNRScores.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND, "Neuron not found in PMNR scores map." );
    }

    return _neuronToPMNRScores[index];
}

std::pair<NeuronIndex, double> NetworkLevelReasoner::getBBPSBranchingPoint( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToBBPSBranchingPoints.empty() )
    {
        initializeBBPSBranchingMaps();
    }

    if ( !_neuronToBBPSBranchingPoints.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND,
                        "Neuron not found in BBPS branching points map." );
    }

    return _neuronToBBPSBranchingPoints[index];
}

Vector<double> NetworkLevelReasoner::getSymbolicLbPerBranch( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToSymbolicLbPerBranch.empty() )
    {
        initializeBBPSBranchingMaps();
    }

    if ( !_neuronToSymbolicLbPerBranch.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND,
                        "Neuron not found in BBPS branch symbolic bounds map." );
    }

    return _neuronToSymbolicLbPerBranch[index];
}

Vector<double> NetworkLevelReasoner::getSymbolicUbPerBranch( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToSymbolicUbPerBranch.empty() )
    {
        initializeBBPSBranchingMaps();
    }

    if ( !_neuronToSymbolicUbPerBranch.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND,
                        "Neuron not found in BBPS branch symbolic bounds map." );
    }

    return _neuronToSymbolicUbPerBranch[index];
}

Vector<double> NetworkLevelReasoner::getSymbolicLowerBiasPerBranch( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToSymbolicLowerBiasPerBranch.empty() )
    {
        initializeBBPSBranchingMaps();
    }

    if ( !_neuronToSymbolicLowerBiasPerBranch.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND,
                        "Neuron not found in BBPS branch symbolic bounds map." );
    }

    return _neuronToSymbolicLowerBiasPerBranch[index];
}

Vector<double> NetworkLevelReasoner::getSymbolicUpperBiasPerBranch( NeuronIndex index )
{
    // Initialize map if empty.
    if ( _neuronToSymbolicUpperBiasPerBranch.empty() )
    {
        initializeBBPSBranchingMaps();
    }

    if ( !_neuronToSymbolicUpperBiasPerBranch.exists( index ) )
    {
        throw NLRError( NLRError::NEURON_NOT_FOUND,
                        "Neuron not found in BBPS branch symbolic bounds map." );
    }

    return _neuronToSymbolicUpperBiasPerBranch[index];
}

unsigned
NetworkLevelReasoner::mergeConsecutiveWSLayers( const Map<unsigned, double> &lowerBounds,
                                                const Map<unsigned, double> &upperBounds,
                                                const Set<unsigned> &varsInUnhandledConstraints,
                                                Map<unsigned, LinearExpression> &eliminatedNeurons )
{
    // Iterate over all layers, except the input layer
    unsigned layer = 1;

    unsigned numberOfMergedLayers = 0;
    while ( layer < _layerIndexToLayer.size() )
    {
        if ( suitableForMerging( layer, lowerBounds, upperBounds, varsInUnhandledConstraints ) )
        {
            NLR_LOG( Stringf( "Merging layer %u with its predecessor...", layer ).ascii() );
            mergeWSLayers( layer, eliminatedNeurons );
            ++numberOfMergedLayers;
            NLR_LOG( Stringf( "Merging layer %u with its predecessor - done", layer ).ascii() );
        }
        else
            ++layer;
    }
    return numberOfMergedLayers;
}

bool NetworkLevelReasoner::suitableForMerging(
    unsigned secondLayerIndex,
    const Map<unsigned, double> &lowerBounds,
    const Map<unsigned, double> &upperBounds,
    const Set<unsigned> &varsInConstraintsUnhandledByNLR )
{
    NLR_LOG( Stringf( "Checking whether layer %u is suitable for merging...", secondLayerIndex )
                 .ascii() );

    /*
      The given layer index is a candidate layer. We now check whether
      it is an eligible second WS layer that can be merged with its
      predecessor
    */
    const Layer *secondLayer = _layerIndexToLayer[secondLayerIndex];

    // Layer should be a Weighted Sum layer
    if ( secondLayer->getLayerType() != Layer::WEIGHTED_SUM )
        return false;

    // Layer should have a single source
    if ( secondLayer->getSourceLayers().size() != 1 )
        return false;

    // Grab the predecessor layer
    unsigned firstLayerIndex = secondLayer->getSourceLayers().begin()->first;
    const Layer *firstLayer = _layerIndexToLayer[firstLayerIndex];

    // First layer should also be a weighted sum
    if ( firstLayer->getLayerType() != Layer::WEIGHTED_SUM )
        return false;

    // First layer should not feed into any other layer
    unsigned count = 0;
    for ( unsigned i = 0; i < getNumberOfLayers(); ++i )
    {
        const Layer *layer = _layerIndexToLayer[i];

        if ( layer->getSourceLayers().exists( firstLayerIndex ) )
            ++count;
    }
    if ( count > 1 )
        return false;

    // If there are bounds on the predecessor layer or if
    // the predecessor layer participates in any constraints
    // (equations, piecewise-linear constraints, nonlinear-constraints)
    // unaccounted for in the NLR cannot merge
    for ( unsigned i = 0; i < firstLayer->getSize(); ++i )
    {
        unsigned variable = firstLayer->neuronToVariable( i );
        if ( ( lowerBounds.exists( variable ) && FloatUtils::isFinite( lowerBounds[variable] ) ) ||
             ( upperBounds.exists( variable ) && FloatUtils::isFinite( upperBounds[variable] ) ) ||
             varsInConstraintsUnhandledByNLR.exists( variable ) )
            return false;
    }
    return true;
}

const Vector<double> NetworkLevelReasoner::OptimalParameterisedSymbolicBoundTightening()
{
    // Search over coeffs in [0, 1]^number_of_parameters with projected grdient descent.
    unsigned maxIterations =
        GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_MAX_ITERATIONS;
    double stepSize = GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_STEP_SIZE;
    double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS;
    double weightDecay = GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_WEIGHT_DECAY;
    double lr = GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_LEARNING_RATE;
    unsigned dimension = getNumberOfParameters();
    bool maximize = false;
    double sign = ( maximize ? 1 : -1 );

    Vector<double> lowerBounds( dimension, 0 );
    Vector<double> upperBounds( dimension, 1 );

    // Initialize initial guess uniformly.
    Vector<double> guess( dimension );
    std::mt19937_64 rng( GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_RANDOM_SEED );
    std::uniform_real_distribution<double> dis( 0, 1 );
    for ( unsigned j = 0; j < dimension; ++j )
    {
        double lb = lowerBounds[j];
        double ub = upperBounds[j];
        guess[j] = lb + dis( rng ) * ( ub - lb );
    }

    Vector<Vector<double>> candidates( dimension );
    Vector<double> gradient( dimension );

    for ( unsigned i = 0; i < maxIterations; ++i )
    {
        double currentCost = EstimateVolume( guess );
        for ( unsigned j = 0; j < dimension; ++j )
        {
            candidates[j] = Vector<double>( guess );
            candidates[j][j] += stepSize;

            if ( candidates[j][j] > upperBounds[j] || candidates[j][j] < lowerBounds[j] )
            {
                gradient[j] = 0;
                continue;
            }

            double cost = EstimateVolume( candidates[j] );
            gradient[j] = ( cost - currentCost ) / stepSize + weightDecay * guess[j];
        }

        bool gradientIsZero = true;
        for ( unsigned j = 0; j < dimension; ++j )
        {
            if ( FloatUtils::abs( gradient[j] ) > epsilon )
            {
                gradientIsZero = false;
            }
        }
        if ( gradientIsZero )
        {
            break;
        }

        for ( unsigned j = 0; j < dimension; ++j )
        {
            guess[j] += sign * lr * gradient[j];

            guess[j] = std::min( guess[j], upperBounds[j] );
            guess[j] = std::max( guess[j], lowerBounds[j] );
        }
    }

    const Vector<double> optimalCoeffs( guess );
    return optimalCoeffs;
}

double NetworkLevelReasoner::EstimateVolume( const Vector<double> &coeffs )
{
    // First, run parameterised symbolic bound propagation.
    Map<unsigned, Vector<double>> layerIndicesToParameters = getParametersForLayers( coeffs );
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
    {
        ASSERT( _layerIndexToLayer.exists( i ) );
        const Vector<double> &currentLayerCoeffs = layerIndicesToParameters[i];
        _layerIndexToLayer[i]->computeParameterisedSymbolicBounds( currentLayerCoeffs );
    }

    std::mt19937_64 rng( GlobalConfiguration::VOLUME_ESTIMATION_RANDOM_SEED );
    double logBoxVolume = 0;
    double sigmoidSum = 0;

    unsigned inputLayerIndex = 0;
    unsigned outputLayerIndex = _layerIndexToLayer.size() - 1;
    Layer *inputLayer = _layerIndexToLayer[inputLayerIndex];
    Layer *outputLayer = _layerIndexToLayer[outputLayerIndex];

    // Calculate volume of input variables' bounding box.
    for ( unsigned index = 0; index < inputLayer->getSize(); ++index )
    {
        if ( inputLayer->neuronEliminated( index ) )
            continue;

        double lb = inputLayer->getLb( index );
        double ub = inputLayer->getUb( index );

        if ( lb == ub )
            continue;

        logBoxVolume += std::log( ub - lb );
    }

    for ( unsigned i = 0; i < GlobalConfiguration::VOLUME_ESTIMATION_ITERATIONS; ++i )
    {
        // Sample input point from known bounds.
        Map<unsigned, double> point;
        for ( unsigned j = 0; j < inputLayer->getSize(); ++j )
        {
            if ( inputLayer->neuronEliminated( j ) )
            {
                point.insert( j, 0 );
            }
            else
            {
                double lb = inputLayer->getLb( j );
                double ub = inputLayer->getUb( j );
                std::uniform_real_distribution<> dis( lb, ub );
                point.insert( j, dis( rng ) );
            }
        }

        // Calculate sigmoid of maximum margin from output symbolic bounds.
        double maxMargin = 0;
        for ( unsigned j = 0; j < outputLayer->getSize(); ++j )
        {
            if ( outputLayer->neuronEliminated( j ) )
                continue;

            double margin = calculateDifferenceFromSymbolic( outputLayer, point, j );
            maxMargin = std::max( maxMargin, margin );
        }
        sigmoidSum += SigmoidConstraint::sigmoid( maxMargin );
    }

    return std::exp( logBoxVolume + std::log( sigmoidSum ) ) /
           GlobalConfiguration::VOLUME_ESTIMATION_ITERATIONS;
}

double NetworkLevelReasoner::calculateDifferenceFromSymbolic( const Layer *layer,
                                                              Map<unsigned, double> &point,
                                                              unsigned i ) const
{
    unsigned size = layer->getSize();
    unsigned inputLayerSize = _layerIndexToLayer[0]->getSize();
    double lowerSum = layer->getSymbolicLowerBias()[i];
    double upperSum = layer->getSymbolicUpperBias()[i];

    for ( unsigned j = 0; j < inputLayerSize; ++j )
    {
        lowerSum += layer->getSymbolicLb()[j * size + i] * point[j];
        upperSum += layer->getSymbolicUb()[j * size + i] * point[j];
    }

    return std::max( layer->getUb( i ) - upperSum, lowerSum - layer->getLb( i ) );
}

const Vector<PolygonalTightening> NetworkLevelReasoner::generatePolygonalTighteningsForPMNR()
{
    Vector<PolygonalTightening> tightenings = Vector<PolygonalTightening>( {} );
    Vector<PolygonalTightening> lowerDeepPolyTightenings = Vector<PolygonalTightening>( {} );
    Vector<PolygonalTightening> upperDeepPolyTightenings = Vector<PolygonalTightening>( {} );
    const Vector<NeuronIndex> neurons = selectPMNRNeurons();
    unsigned neuronCount = neurons.size();

    // Initial tightenings are non-fixed neurons and their DeepPoly symbolic bounds.
    for ( const auto &pair : neurons )
    {
        unsigned layerIndex = pair._layer;
        unsigned neuron = pair._neuron;
        Layer *layer = _layerIndexToLayer[layerIndex];
        unsigned size = layer->getSize();

        Map<NeuronIndex, double> lowerCoeffs;
        Map<NeuronIndex, double> upperCoeffs;
        lowerCoeffs[pair] = 1;
        upperCoeffs[pair] = -1;

        unsigned inputIndex = 0;
        List<NeuronIndex> sources = layer->getActivationSources( neuron );
        for ( const auto &sourceIndex : sources )
        {
            lowerCoeffs[sourceIndex] =
                -getPredecessorSymbolicLb( layerIndex )[size * inputIndex + neuron];
            upperCoeffs[sourceIndex] =
                getPredecessorSymbolicUb( layerIndex )[size * inputIndex + neuron];
            ++inputIndex;
        }
        PolygonalTightening lowerTightening( lowerCoeffs,
                                             getPredecessorSymbolicLowerBias( layerIndex )[neuron],
                                             PolygonalTightening::LB );
        PolygonalTightening upperTightening( upperCoeffs,
                                             -getPredecessorSymbolicUpperBias( layerIndex )[neuron],
                                             PolygonalTightening::LB );
        lowerDeepPolyTightenings.append( lowerTightening );
        upperDeepPolyTightenings.append( upperTightening );
    }

    /*
       If DeepPoly bounds are x_f - \sum a_u_i x_i <= b_u, \sum x_f - a_u_i x_i >= b_l, PMNR
       tightenings are linear combinations of x_f - \sum a_u_i x_i, coeffs in {-1, 0, 1}, and
       linear combinations of x_f - \sum a_l_i x_i, coeffs in {-1, 0, 1}.
    */
    const Vector<double> weights = Vector<double>( { -1, 0, 1 } );
    unsigned weightCount = weights.size();
    unsigned range = std::pow( weightCount, neuronCount );
    for ( unsigned i = 0; i < range; ++i )
    {
        // Keep track of whether all coefficients for current tightening are non-negative,
        // and count non-zero coefficients.
        unsigned nonZeroWeights = 0;
        bool allNonnegative = true;
        Map<NeuronIndex, double> lowerCoeffs;
        Map<NeuronIndex, double> upperCoeffs;
        for ( unsigned j = 0; j < neuronCount; ++j )
        {
            unsigned mask = std::pow( weightCount, j );
            unsigned flag = ( i / mask ) % weightCount;
            double weight = weights[flag];
            if ( weight < 0 )
            {
                allNonnegative = false;
            }
            if ( weight != 0 )
            {
                ++nonZeroWeights;
            }

            // Compute linear combinations of DeepPoly tightenings.
            for ( const auto &pair : lowerDeepPolyTightenings[j]._neuronToCoefficient )
            {
                if ( !lowerCoeffs.exists( pair.first ) )
                {
                    lowerCoeffs[pair.first] = weight * pair.second;
                }
                else
                {
                    lowerCoeffs[pair.first] = lowerCoeffs[pair.first] + weight * pair.second;
                }
            }
            for ( const auto &pair : upperDeepPolyTightenings[j]._neuronToCoefficient )
            {
                if ( !upperCoeffs.exists( pair.first ) )
                {
                    upperCoeffs[pair.first] = weight * pair.second;
                }
                else
                {
                    upperCoeffs[pair.first] = upperCoeffs[pair.first] + weight * pair.second;
                }
            }
        }

        // No need to tighten the original DeepPoly bounds.
        if ( nonZeroWeights <= 1 )
        {
            continue;
        }

        // Calculate initial concrete lower bound for all tightenings. If all coefficients are,
        // non-negative, compute lower bounds by adding the DeepPoly tightenings' lower bounds.
        double lowerTighteningLb = 0;
        double upperTighteningLb = 0;
        if ( allNonnegative )
        {
            for ( unsigned j = 0; j < neuronCount; ++j )
            {
                unsigned mask = std::pow( weightCount, j );
                unsigned flag = ( i / mask ) % weightCount;
                double weight = weights[flag];
                lowerTighteningLb += weight * lowerDeepPolyTightenings[j]._value;
                upperTighteningLb += weight * upperDeepPolyTightenings[j]._value;
            }
        }

        // If some weights are negative, compute lower bounds by concretizing.
        else
        {
            for ( const auto &pair : lowerCoeffs )
            {
                double neuronLb =
                    _layerIndexToLayer[pair.first._layer]->getLb( pair.first._neuron );
                double neuronUb =
                    _layerIndexToLayer[pair.first._layer]->getUb( pair.first._neuron );
                lowerTighteningLb +=
                    pair.second >= 0 ? pair.second * neuronLb : pair.second * neuronUb;
            }
            for ( const auto &pair : upperCoeffs )
            {
                double neuronLb =
                    _layerIndexToLayer[pair.first._layer]->getLb( pair.first._neuron );
                double neuronUb =
                    _layerIndexToLayer[pair.first._layer]->getUb( pair.first._neuron );
                upperTighteningLb +=
                    pair.second >= 0 ? pair.second * neuronLb : pair.second * neuronUb;
            }
        }
        PolygonalTightening lowerTightening(
            lowerCoeffs, lowerTighteningLb, PolygonalTightening::LB );
        PolygonalTightening upperTightening(
            upperCoeffs, upperTighteningLb, PolygonalTightening::LB );
        tightenings.append( lowerTightening );
        tightenings.append( upperTightening );
    }

    const Vector<PolygonalTightening> tighteningsVector =
        Vector<PolygonalTightening>( tightenings );
    return tighteningsVector;
}

const Vector<NeuronIndex> NetworkLevelReasoner::selectPMNRNeurons()
{
    // Select layer with maximal PMNR neuron score sum.
    const Vector<unsigned> &candidateLayers = getLayersWithNonfixedNeurons();
    if ( candidateLayers.empty() )
    {
        const Vector<NeuronIndex> emptyVector( {} );
        return emptyVector;
    }

    double maxScore = 0;
    unsigned maxScoreIndex = 0;
    for ( const auto &layerIndex : candidateLayers )
    {
        double layerScore = 0;
        Layer *layer = _layerIndexToLayer[layerIndex];
        for ( const auto &index : layer->getNonfixedNeurons() )
        {
            double neuronScore = getPMNRScore( index );
            layerScore += neuronScore;
        }

        if ( layerScore > maxScore )
        {
            maxScore = layerScore;
            maxScoreIndex = layerIndex;
        }
    }

    // Extract highest score neurons from this layer.
    Layer *layer = _layerIndexToLayer[maxScoreIndex];
    std::priority_queue<std::pair<double, unsigned>,
                        std::vector<std::pair<double, unsigned>>,
                        std::less<std::pair<double, unsigned>>>
        maxQueue;
    const Vector<NeuronIndex> nonfixedNeurons = layer->getNonfixedNeurons();
    for ( const auto &index : nonfixedNeurons )
    {
        maxQueue.push( std::pair( getPMNRScore( index ), index._neuron ) );
    }

    unsigned neuronCount =
        std::min( GlobalConfiguration::PMNR_SELECTED_NEURONS, nonfixedNeurons.size() );
    Vector<NeuronIndex> selectedNeurons = Vector<NeuronIndex>( neuronCount );
    for ( unsigned i = 0; i < neuronCount; ++i )
    {
        selectedNeurons[i] = NeuronIndex( maxScoreIndex, maxQueue.top().second );
        maxQueue.pop();
    }
    const Vector<NeuronIndex> neurons( selectedNeurons );
    return neurons;
}

const Vector<PolygonalTightening> NetworkLevelReasoner::OptimizeParameterisedPolygonalTightening()
{
    // Calculate successor layers, PMNR scores, symbolic bound maps before optimizing.
    computeSuccessorLayers();
    parameterisedDeepPoly( true );
    initializePMNRScoreMap();

    // Repeatedly optimize polygonal tightenings given previously optimized ones.
    const Vector<PolygonalTightening> &selectedTightenings = generatePolygonalTighteningsForPMNR();
    Vector<PolygonalTightening> optimizedTightenings = Vector<PolygonalTightening>( {} );
    for ( unsigned i = 0; i < selectedTightenings.size(); ++i )
    {
        PolygonalTightening tightening = selectedTightenings[i];
        bool maximize = ( tightening._type == PolygonalTightening::LB );
        double feasibiltyBound = maximize ? FloatUtils::infinity() : FloatUtils::negativeInfinity();
        double bound = OptimizeSingleParameterisedPolygonalTightening(
            tightening, optimizedTightenings, maximize, feasibiltyBound );

        // Attempt to obtain stronger bound by branching selected neurons.
        tightening._value = OptimizeSingleParameterisedPolygonalTighteningWithBranching(
            tightening, optimizedTightenings, maximize, bound );
        optimizedTightenings.append( tightening );

        // Store optimized tightenings in NLR.
        receivePolygonalTightening( tightening );
    }

    const Vector<PolygonalTightening> tightenings( optimizedTightenings );
    return tightenings;
}

double NetworkLevelReasoner::OptimizeSingleParameterisedPolygonalTighteningWithBranching(
    PolygonalTightening &tightening,
    Vector<PolygonalTightening> &prevTightenings,
    bool maximize,
    double originalBound )
{
    // Determine which of the selected neurons support branching.
    const Vector<NeuronIndex> selectedNeurons = selectPMNRNeurons();
    Vector<NeuronIndex> neurons = Vector<NeuronIndex>( {} );
    for ( const auto &index : selectedNeurons )
    {
        const Layer *layer = _layerIndexToLayer[index._layer];
        if ( layer->neuronNonfixed( index._neuron ) &&
             supportsInvpropBranching( layer->getLayerType() ) )
        {
            neurons.append( index );
        }
    }
    if ( neurons.empty() )
        return originalBound;

    // Re-optimize current tightening for every branch combination. If we seek to maximize
    // the tightening's bound, select minimal score of all combinations, and vice versa.
    bool maximizeBranchBound = !maximize;
    double newBound = maximizeBranchBound ? FloatUtils::negativeInfinity() : FloatUtils::infinity();
    unsigned neuronCount = neurons.size();
    Vector<unsigned> branchCounts( neuronCount, 0 );
    for ( unsigned i = 0; i < neuronCount; ++i )
    {
        branchCounts[i] = getSymbolicLbPerBranch( neurons[i] ).size();
    }
    unsigned range =
        std::accumulate( branchCounts.begin(), branchCounts.end(), 1, std::multiplies<unsigned>() );
    for ( unsigned i = 0; i < range; ++i )
    {
        Map<NeuronIndex, unsigned> neuronToBranchIndex;
        for ( unsigned j = 0; j < neuronCount; ++j )
        {
            unsigned mask = std::accumulate( branchCounts.begin(),
                                             std::next( branchCounts.begin(), j ),
                                             1,
                                             std::multiplies<unsigned>() );
            unsigned branchIndex = ( i / mask ) % branchCounts[j];
            NeuronIndex index = neurons[j];
            neuronToBranchIndex.insert( index, branchIndex );
        }

        // To determine some of the infeasible branch combinations, calculate a feasibility bound
        // (known upper/lower bound for max/min problem) with concretization.
        double feasibilityBound = 0;
        for ( const auto &pair : tightening._neuronToCoefficient )
        {
            double ub = _layerIndexToLayer[pair.first._layer]->getUb( pair.first._neuron );
            double lb = _layerIndexToLayer[pair.first._layer]->getLb( pair.first._neuron );
            if ( maximize )
            {
                feasibilityBound += pair.second > 0 ? pair.second * ub : pair.second * lb;
            }
            else
            {
                feasibilityBound += pair.second > 0 ? pair.second * lb : pair.second * ub;
            }
        }

        double branchBound = OptimizeSingleParameterisedPolygonalTightening(
            tightening, prevTightenings, maximize, feasibilityBound, neuronToBranchIndex );

        // If bound is stronger than known feasibility bound, store branch combination in NLR.
        if ( !FloatUtils::isFinite( branchBound ) || maximize ? branchBound > feasibilityBound
                                                              : branchBound < feasibilityBound )
        {
            receiveInfeasibleBranches( neuronToBranchIndex );
        }
        else
        {
            newBound = maximizeBranchBound ? std::max( branchBound, newBound )
                                           : std::min( branchBound, newBound );
        }
    }

    newBound = maximize ? std::max( originalBound, newBound ) : std::min( originalBound, newBound );
    return newBound;
}

double NetworkLevelReasoner::OptimizeSingleParameterisedPolygonalTightening(
    PolygonalTightening &tightening,
    Vector<PolygonalTightening> &prevTightenings,
    bool maximize,
    double feasibilityBound,
    const Map<NeuronIndex, unsigned> &neuronToBranchIndex )
{
    // Search over gamma in [0, inf)^sizeOfPrevTightenings with PGD.
    unsigned maxIterations = GlobalConfiguration::INVPROP_MAX_ITERATIONS;
    double gammaStepSize = GlobalConfiguration::INVPROP_STEP_SIZE;
    double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS;
    double weightDecay = GlobalConfiguration::INVPROP_WEIGHT_DECAY;
    double lr = GlobalConfiguration::INVPROP_LEARNING_RATE;
    unsigned gammaDimension = prevTightenings.size();
    double sign = ( maximize ? 1 : -1 );
    double bestBound = tightening._value;

    Vector<double> gammaLowerBounds( gammaDimension, 0 );

    Vector<double> gamma( gammaDimension, GlobalConfiguration::INVPROP_INITIAL_GAMMA );
    Vector<double> previousGamma( gamma );

    Vector<Vector<double>> gammaCandidates( gammaDimension );
    Vector<double> gammaGradient( gammaDimension );

    for ( unsigned i = 0; i < maxIterations; ++i )
    {
        for ( unsigned j = 0; j < gammaDimension; ++j )
        {
            gamma[j] += weightDecay * ( gamma[j] - previousGamma[j] );
            gamma[j] = std::max( gamma[j], gammaLowerBounds[j] );
        }

        double currentCost = getParameterisdPolygonalTighteningBound(
            gamma, tightening, prevTightenings, neuronToBranchIndex );

        // If calculated bound is stronger than known feasibility bound, stop optimization.
        if ( !FloatUtils::isFinite( currentCost ) || maximize ? currentCost > feasibilityBound
                                                              : currentCost < feasibilityBound )
        {
            return currentCost;
        }

        for ( unsigned j = 0; j < gammaDimension; ++j )
        {
            gammaCandidates[j] = Vector<double>( gamma );
            gammaCandidates[j][j] += gammaStepSize;
            if ( gammaCandidates[j][j] < gammaLowerBounds[j] )
            {
                gammaGradient[j] = 0;
                continue;
            }

            double cost = getParameterisdPolygonalTighteningBound(
                gammaCandidates[j], tightening, prevTightenings, neuronToBranchIndex );
            if ( !FloatUtils::isFinite( cost ) || maximize ? cost > feasibilityBound
                                                           : cost < feasibilityBound )
            {
                return cost;
            }

            gammaGradient[j] = ( cost - currentCost ) / gammaStepSize;
            bestBound = ( maximize ? std::max( bestBound, cost ) : std::min( bestBound, cost ) );
        }

        bool gradientIsZero = true;
        for ( unsigned j = 0; j < gammaDimension; ++j )
        {
            if ( FloatUtils::abs( gammaGradient[j] ) > epsilon )
            {
                gradientIsZero = false;
            }
        }
        if ( gradientIsZero )
        {
            break;
        }
        for ( unsigned j = 0; j < gammaDimension; ++j )
        {
            previousGamma[j] = gamma[j];
            gamma[j] += sign * lr * gammaGradient[j];
            gamma[j] = std::max( gamma[j], gammaLowerBounds[j] );
        }
    }

    return bestBound;
}

double NetworkLevelReasoner::getParameterisdPolygonalTighteningBound(
    const Vector<double> &gamma,
    PolygonalTightening &tightening,
    Vector<PolygonalTightening> &prevTightenings,
    const Map<NeuronIndex, unsigned> &neuronToBranchIndex )
{
    // Recursively compute vectors mu, muHat for every layer with the backpropagation procedure.
    unsigned numLayers = _layerIndexToLayer.size();
    unsigned maxLayer = _layerIndexToLayer.size() - 1;
    unsigned prevTigheningsCount = prevTightenings.size();
    unsigned inputLayerSize = _layerIndexToLayer[0]->getSize();
    double sign = ( tightening._type == PolygonalTightening::LB ? 1 : -1 );
    Vector<Vector<double>> mu( numLayers );
    Vector<Vector<double>> muHat( numLayers );

    for ( unsigned layerIndex = numLayers; layerIndex-- > 0; )
    {
        Layer *layer = _layerIndexToLayer[layerIndex];
        unsigned size = layer->getSize();
        mu[layerIndex] = Vector<double>( size, 0 );
        muHat[layerIndex] = Vector<double>( size, 0 );

        if ( layerIndex < maxLayer )
        {
            for ( unsigned i = 0; i < size; ++i )
            {
                for ( unsigned successorIndex : layer->getSuccessorLayers() )
                {
                    const Layer *successorLayer = _layerIndexToLayer[successorIndex];
                    unsigned successorSize = successorLayer->getSize();

                    if ( successorLayer->getLayerType() == Layer::WEIGHTED_SUM )
                    {
                        const double *weights = successorLayer->getWeightMatrix( layerIndex );
                        for ( unsigned j = 0; j < successorSize; ++j )
                        {
                            if ( !successorLayer->neuronEliminated( j ) )
                            {
                                muHat[layerIndex][i] +=
                                    mu[successorIndex][j] * weights[i * successorSize + j];
                            }
                        }
                    }
                    else
                    {
                        for ( unsigned j = 0; j < successorSize; ++j )
                        {
                            // Find the index of the current neuron in the successor's activation
                            // sources list.
                            bool found = false;
                            unsigned inputIndex = 0;
                            List<NeuronIndex> sources = successorLayer->getActivationSources( j );
                            for ( const auto &sourceIndex : sources )
                            {
                                if ( sourceIndex._layer == layerIndex && sourceIndex._neuron == i )
                                {
                                    found = true;
                                    break;
                                }
                                ++inputIndex;
                            }
                            NeuronIndex successor( successorIndex, j );
                            if ( found )
                            {
                                if ( !successorLayer->neuronEliminated( j ) )
                                {
                                    // When branching selected neurons, use predecessor symbolic
                                    // bounds for current branch.
                                    if ( neuronToBranchIndex.exists( successor ) )
                                    {
                                        muHat[layerIndex][i] +=
                                            std::max( mu[successorIndex][j], 0.0 ) *
                                            getSymbolicUbPerBranch(
                                                successor )[neuronToBranchIndex[successor]];

                                        muHat[layerIndex][i] -=
                                            std::max( -mu[successorIndex][j], 0.0 ) *
                                            getSymbolicLbPerBranch(
                                                successor )[neuronToBranchIndex[successor]];
                                    }
                                    else
                                    {
                                        muHat[layerIndex][i] +=
                                            std::max( mu[successorIndex][j], 0.0 ) *
                                            getPredecessorSymbolicUb(
                                                successorIndex )[successorSize * inputIndex + j];

                                        muHat[layerIndex][i] -=
                                            std::max( -mu[successorIndex][j], 0.0 ) *
                                            getPredecessorSymbolicLb(
                                                successorIndex )[successorSize * inputIndex + j];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if ( layerIndex > 0 )
        {
            // Compute mu from muHat.
            for ( unsigned i = 0; i < size; ++i )
            {
                mu[layerIndex][i] += muHat[layerIndex][i] -
                                     sign * tightening.getCoeff( NeuronIndex( layerIndex, i ) );
                for ( unsigned j = 0; j < prevTigheningsCount; ++j )
                {
                    PolygonalTightening pt = prevTightenings[j];
                    double prevCoeff = pt.getCoeff( NeuronIndex( layerIndex, i ) );
                    double currentSign = ( pt._type == PolygonalTightening::LB ? 1 : -1 );
                    mu[layerIndex][i] += currentSign * gamma[j] * prevCoeff;
                }
            }
        }
    }

    // Compute global bound for input space minimization problem.
    Vector<double> inputLayerBound( inputLayerSize, 0 );
    for ( unsigned i = 0; i < inputLayerSize; ++i )
    {
        inputLayerBound[i] += sign * tightening.getCoeff( NeuronIndex( 0, i ) ) - muHat[0][i];
        for ( unsigned j = 0; j < prevTigheningsCount; ++j )
        {
            PolygonalTightening pt = prevTightenings[j];
            double prevCoeff = pt.getCoeff( NeuronIndex( 0, i ) );
            double currentSign = ( pt._type == PolygonalTightening::LB ? 1 : -1 );
            inputLayerBound[i] -= currentSign * gamma[j] * prevCoeff;
        }
    }

    // Compute bound for polygonal tightening bias using mu and inputLayerBound.
    double bound = 0;
    for ( unsigned i = 0; i < prevTigheningsCount; ++i )
    {
        PolygonalTightening pt = prevTightenings[i];
        double currentSign = ( pt._type == PolygonalTightening::LB ? 1 : -1 );
        bound += currentSign * gamma[i] * pt._value;
    }

    for ( unsigned layerIndex = maxLayer; layerIndex >= 1; --layerIndex )
    {
        Layer *layer = _layerIndexToLayer[layerIndex];
        if ( layer->getLayerType() == Layer::WEIGHTED_SUM )
        {
            const double *biases = layer->getBiases();
            for ( unsigned i = 0; i < layer->getSize(); ++i )
            {
                if ( !layer->neuronEliminated( i ) )
                {
                    bound -= mu[layerIndex][i] * biases[i];
                }
                else
                {
                    bound -= mu[layerIndex][i] * layer->getEliminatedNeuronValue( i );
                }
            }
        }
        else
        {
            for ( unsigned i = 0; i < layer->getSize(); ++i )
            {
                if ( !layer->neuronEliminated( i ) )
                {
                    NeuronIndex index( layerIndex, i );
                    if ( neuronToBranchIndex.exists( index ) )
                    {
                        bound -= std::max( mu[layerIndex][i], 0.0 ) *
                                 getSymbolicUpperBiasPerBranch( index )[neuronToBranchIndex[index]];
                        bound += std::max( -mu[layerIndex][i], 0.0 ) *
                                 getSymbolicLowerBiasPerBranch( index )[neuronToBranchIndex[index]];
                    }
                    else
                    {
                        bound -= std::max( mu[layerIndex][i], 0.0 ) *
                                 getPredecessorSymbolicUpperBias( layerIndex )[i];
                        bound += std::max( -mu[layerIndex][i], 0.0 ) *
                                 getPredecessorSymbolicLowerBias( layerIndex )[i];
                    }
                }
                else
                {
                    bound -=
                        FloatUtils::abs( mu[layerIndex][i] ) * layer->getEliminatedNeuronValue( i );
                }
            }
        }
    }

    Layer *inputLayer = _layerIndexToLayer[0];
    for ( unsigned i = 0; i < inputLayerSize; ++i )
    {
        bound += std::max( inputLayerBound[i], 0.0 ) * inputLayer->getLb( i );
        bound -= std::max( -inputLayerBound[i], 0.0 ) * inputLayer->getUb( i );
    }
    return sign * bound;
}

void NetworkLevelReasoner::initializePMNRScoreMap()
{
    // Clear PMNR score map.
    _neuronToPMNRScores.clear();
    for ( const auto &pair : _layerIndexToLayer )
    {
        for ( const auto &index : pair.second->getNonfixedNeurons() )
        {
            _neuronToPMNRScores.insert( index, calculatePMNRBBPSScore( index ) );
        }
    }
}

double NetworkLevelReasoner::calculatePMNRBBPSScore( NeuronIndex index )
{
    // Initialize BBPS branching points and branch symbolic bound maps.
    initializeBBPSBranchingMaps();

    Layer *outputLayer = _layerIndexToLayer[getNumberOfLayers() - 1];
    unsigned outputLayerSize = outputLayer->getSize();
    unsigned layerIndex = index._layer;
    unsigned neuron = index._neuron;
    Layer *layer = _layerIndexToLayer[layerIndex];

    // We have the symbolic bounds map of the output layer in terms of the given neuron's layer.
    // Concretize all neurons except from the given neuron.
    Vector<double> concretizedOutputSymbolicLb( outputLayerSize, 0 );
    Vector<double> concretizedOutputSymbolicUb( outputLayerSize, 0 );
    Vector<double> concretizedOutputSymbolicLowerBias( outputLayerSize, 0 );
    Vector<double> concretizedOutputSymbolicUpperBias( outputLayerSize, 0 );
    for ( unsigned i = 0; i < outputLayerSize; ++i )
    {
        concretizedOutputSymbolicLb[i] =
            getOutputSymbolicLb( layerIndex )[outputLayerSize * neuron + i];
        concretizedOutputSymbolicUb[i] =
            getOutputSymbolicUb( layerIndex )[outputLayerSize * neuron + i];
        concretizedOutputSymbolicLowerBias[i] = getOutputSymbolicLowerBias( layerIndex )[i];
        concretizedOutputSymbolicUpperBias[i] = getOutputSymbolicUpperBias( layerIndex )[i];

        for ( unsigned j = 0; j < layer->getSize(); ++j )
        {
            if ( j != neuron )
            {
                double lowerWeight = getOutputSymbolicLb( layerIndex )[outputLayerSize * j + i];
                double upperWeight = getOutputSymbolicUb( layerIndex )[outputLayerSize * j + i];
                concretizedOutputSymbolicLowerBias[i] += lowerWeight > 0
                                                           ? lowerWeight * layer->getLb( j )
                                                           : lowerWeight * layer->getUb( j );
                concretizedOutputSymbolicUpperBias[i] += upperWeight > 0
                                                           ? upperWeight * layer->getUb( j )
                                                           : upperWeight * layer->getLb( j );
            }
        }
    }

    // For every branch, we calculated the output layer's symbolic bounds in terms of the given
    // neuron, and branch symbolic bounds of this neuron in terms of one source neuron.
    std::pair<NeuronIndex, double> point = getBBPSBranchingPoint( index );
    NeuronIndex sourceIndex = point.first;
    const Layer *sourceLayer = getLayer( sourceIndex._layer );
    double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
    double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

    Vector<double> symbolicLbPerBranch = getSymbolicLbPerBranch( index );
    Vector<double> symbolicUbPerBranch = getSymbolicUbPerBranch( index );
    Vector<double> symbolicLowerBiasPerBranch = getSymbolicLowerBiasPerBranch( index );
    Vector<double> symbolicUpperBiasPerBranch = getSymbolicUpperBiasPerBranch( index );

    unsigned branchCount = symbolicLbPerBranch.size();
    ASSERT( symbolicUbPerBranch.size() == branchCount );
    ASSERT( symbolicLowerBiasPerBranch.size() == branchCount );
    ASSERT( symbolicUpperBiasPerBranch.size() == branchCount );

    // For every output neuron, substitute branch symbolic bounds in the output symbolic bounds,
    // sum over all branches and output bounds.
    double sourceSymbolicLb = 0;
    double sourceSymbolicUb = 0;
    double sourceSymbolicLowerBias = 0;
    double sourceSymbolicUpperBias = 0;
    for ( unsigned i = 0; i < outputLayerSize; ++i )
    {
        for ( unsigned j = 0; j < branchCount; ++j )
        {
            sourceSymbolicLowerBias += concretizedOutputSymbolicLowerBias[i];
            sourceSymbolicUpperBias += concretizedOutputSymbolicUpperBias[i];

            if ( concretizedOutputSymbolicLb[i] > 0 )
            {
                sourceSymbolicLb += concretizedOutputSymbolicLb[i] * symbolicLbPerBranch[j];
                sourceSymbolicLowerBias +=
                    concretizedOutputSymbolicLb[i] * symbolicLowerBiasPerBranch[j];
            }
            else
            {
                sourceSymbolicLb += concretizedOutputSymbolicLb[i] * symbolicUbPerBranch[j];
                sourceSymbolicLowerBias +=
                    concretizedOutputSymbolicLb[i] * symbolicUpperBiasPerBranch[j];
            }

            if ( concretizedOutputSymbolicUb[i] > 0 )
            {
                sourceSymbolicUb += concretizedOutputSymbolicUb[i] * symbolicUbPerBranch[j];
                sourceSymbolicUpperBias +=
                    concretizedOutputSymbolicUb[i] * symbolicUpperBiasPerBranch[j];
            }
            else
            {
                sourceSymbolicUb += concretizedOutputSymbolicUb[i] * symbolicLbPerBranch[j];
                sourceSymbolicUpperBias +=
                    concretizedOutputSymbolicUb[i] * symbolicLowerBiasPerBranch[j];
            }
        }
    }

    // Concretize the source neuron to get upper and lower bounds for the symbolic expression.
    // The neuron's final score is the range of the concrete bounds divided by the branch count.
    double scoreLower = sourceSymbolicLb > 0
                          ? sourceSymbolicLb * sourceLb + sourceSymbolicLowerBias
                          : sourceSymbolicLb * sourceUb + sourceSymbolicLowerBias;
    double scoreUpper = sourceSymbolicUb > 0
                          ? sourceSymbolicUb * sourceUb + sourceSymbolicUpperBias
                          : sourceSymbolicUb * sourceLb + sourceSymbolicUpperBias;

    return ( scoreUpper - scoreLower ) / branchCount;
}

void NetworkLevelReasoner::initializeBBPSBranchingMaps()
{
    // Clear BBPS branching points and branch symbolic bound maps.
    _neuronToBBPSBranchingPoints.clear();
    _neuronToSymbolicLbPerBranch.clear();
    _neuronToSymbolicUbPerBranch.clear();
    _neuronToSymbolicLowerBiasPerBranch.clear();
    _neuronToSymbolicUpperBiasPerBranch.clear();

    // Calculate branching points, symbolic bounds for non-fixed neurons which support branching.
    for ( const auto &pair : _layerIndexToLayer )
    {
        const Layer *layer = pair.second;
        for ( const auto &index : layer->getNonfixedNeurons() )
        {
            if ( supportsInvpropBranching( layer->getLayerType() ) )
            {
                std::pair<NeuronIndex, double> point = calculateBranchingPoint( index );
                _neuronToBBPSBranchingPoints.insert( index, point );

                NeuronIndex sourceIndex = point.first;
                double value = point.second;
                const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
                double sourceUb = sourceLayer->getUb( sourceIndex._neuron );
                const Vector<double> values = Vector<double>( { sourceLb, value, sourceUb } );

                unsigned branchCount = values.size() - 1;
                Vector<double> symbolicLbPerBranch = Vector<double>( branchCount, 0 );
                Vector<double> symbolicUbPerBranch = Vector<double>( branchCount, 0 );
                Vector<double> symbolicLowerBiasPerBranch = Vector<double>( branchCount, 0 );
                Vector<double> symbolicUpperBiasPerBranch = Vector<double>( branchCount, 0 );

                calculateSymbolicBoundsPerBranch( index,
                                                  sourceIndex,
                                                  values,
                                                  symbolicLbPerBranch,
                                                  symbolicUbPerBranch,
                                                  symbolicLowerBiasPerBranch,
                                                  symbolicUpperBiasPerBranch,
                                                  branchCount );

                _neuronToSymbolicLbPerBranch.insert( index, symbolicLbPerBranch );
                _neuronToSymbolicUbPerBranch.insert( index, symbolicUbPerBranch );
                _neuronToSymbolicLowerBiasPerBranch.insert( index, symbolicLowerBiasPerBranch );
                _neuronToSymbolicUpperBiasPerBranch.insert( index, symbolicUpperBiasPerBranch );
            }
        }
    }
}

const std::pair<NeuronIndex, double>
NetworkLevelReasoner::calculateBranchingPoint( NeuronIndex index ) const
{
    const Layer *layer = _layerIndexToLayer[index._layer];
    unsigned neuron = index._neuron;
    ASSERT( layer->neuronNonfixed( neuron ) );
    std::pair<NeuronIndex, double> point;

    // Heuristically generate candidates for branching points.
    Vector<std::pair<NeuronIndex, double>> candidates =
        generateBranchingPointCandidates( layer, neuron );
    unsigned numberOfCandidates = candidates.size();

    if ( numberOfCandidates == 1 )
    {
        point = candidates[0];
    }
    else
    {
        Vector<double> scores( numberOfCandidates, 0 );
        double minScore = FloatUtils::infinity();
        unsigned minScoreIndex = 0;
        for ( unsigned i = 0; i < numberOfCandidates; ++i )
        {
            // Calculate branch symbolic bounds for every candidate.
            NeuronIndex sourceIndex = candidates[i].first;
            double value = candidates[i].second;
            const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
            double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
            double sourceUb = sourceLayer->getUb( sourceIndex._neuron );
            const Vector<double> values = Vector<double>( { sourceLb, value, sourceUb } );

            unsigned branchCount = values.size() - 1;
            Vector<double> symbolicLbPerBranch = Vector<double>( branchCount, 0 );
            Vector<double> symbolicUbPerBranch = Vector<double>( branchCount, 0 );
            Vector<double> symbolicLowerBiasPerBranch = Vector<double>( branchCount, 0 );
            Vector<double> symbolicUpperBiasPerBranch = Vector<double>( branchCount, 0 );
            calculateSymbolicBoundsPerBranch( index,
                                              sourceIndex,
                                              values,
                                              symbolicLbPerBranch,
                                              symbolicUbPerBranch,
                                              symbolicLowerBiasPerBranch,
                                              symbolicUpperBiasPerBranch,
                                              branchCount );

            // Select candidate which minimizes tightening loss.
            scores[i] = calculateTighteningLoss( values,
                                                 symbolicLbPerBranch,
                                                 symbolicUbPerBranch,
                                                 symbolicLowerBiasPerBranch,
                                                 symbolicUpperBiasPerBranch,
                                                 branchCount );
            if ( scores[i] < minScore )
            {
                minScore = scores[i];
                minScoreIndex = i;
            }
        }
        point = candidates[minScoreIndex];
    }

    const std::pair<NeuronIndex, double> branchingPoint( point );
    return branchingPoint;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidates( const Layer *layer, unsigned i ) const
{
    ASSERT( layer->neuronNonfixed( i ) );
    Layer::Type type = layer->getLayerType();

    switch ( type )
    {
    case Layer::RELU:
    case Layer::LEAKY_RELU:
    case Layer::SIGN:
    case Layer::ABSOLUTE_VALUE:
    {
        return generateBranchingPointCandidatesAtZero( layer, i );
        break;
    }
    case Layer::ROUND:
    {
        return generateBranchingPointCandidatesForRound( layer, i );
        break;
    }
    case Layer::SIGMOID:
    {
        return generateBranchingPointCandidatesForSigmoid( layer, i );
        break;
    }
    case Layer::MAX:
    {
        return generateBranchingPointCandidatesForMax( layer, i );
        break;
    }
    case Layer::SOFTMAX:
    {
        return generateBranchingPointCandidatesForSoftmax( layer, i );
        break;
    }
    case Layer::BILINEAR:
    {
        return generateBranchingPointCandidatesForBilinear( layer, i );
        break;
    }
    default:
    {
        printf( "Error! Neuron type %u unsupported\n", type );
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
        break;
    }
    }
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesAtZero( const Layer *layer, unsigned i ) const
{
    // A Relu/Sign/Abs/Leaky Relu activation is only branched at zero.
    Vector<std::pair<NeuronIndex, double>> candidates;
    NeuronIndex sourceIndex = *layer->getActivationSources( i ).begin();
    std::pair<NeuronIndex, double> point( sourceIndex, 0 );
    candidates.append( point );
    const Vector<std ::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesForRound( const Layer *layer,
                                                                unsigned i ) const
{
    // For a Round activation, the two candidates are selected as the highest value which
    // rounds to the source's lb, and the lowest value which rounds to the source's ub.
    Vector<std::pair<NeuronIndex, double>> candidates;
    NeuronIndex sourceIndex = *layer->getActivationSources( i ).begin();
    const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
    double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
    double sourceUb = sourceLayer->getUb( sourceIndex._neuron );
    std::pair<NeuronIndex, double> pointLower(
        sourceIndex,
        FloatUtils::round( sourceLb ) + 0.5 -
            GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    std::pair<NeuronIndex, double> pointUpper(
        sourceIndex,
        FloatUtils::round( sourceUb ) - 0.5 +
            GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    candidates.append( pointLower );
    candidates.append( pointUpper );
    const Vector<std::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesForSigmoid( const Layer *layer,
                                                                  unsigned i ) const
{
    // For a Sigmoid activation, sample candidates uniformly in [sourceLb, sourceUb].
    Vector<std::pair<NeuronIndex, double>> candidates;
    NeuronIndex sourceIndex = *layer->getActivationSources( i ).begin();
    const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
    double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
    double sourceUb = sourceLayer->getUb( sourceIndex._neuron );
    unsigned numberOfCandidates = GlobalConfiguration::PMNR_BBPS_BRANCHING_CANDIDATES;
    for ( unsigned j = 0; j < numberOfCandidates; ++j )
    {
        std::pair<NeuronIndex, double> point( sourceIndex,
                                              sourceLb + ( j + 1 ) * ( sourceUb - sourceLb ) /
                                                             ( numberOfCandidates + 1 ) );
        candidates.append( point );
    }
    const Vector<std::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesForMax( const Layer *layer, unsigned i ) const
{
    // For a Max activation, calculate source index of largest lower bound
    // and sample candidates uniformly in [sourceLb, sourceUb].
    Vector<std::pair<NeuronIndex, double>> candidates;
    List<NeuronIndex> sources = layer->getActivationSources( i );
    NeuronIndex indexOfMaxLowerBound = *( sources.begin() );
    double maxLowerBound = FloatUtils::negativeInfinity();
    for ( const auto &sourceIndex : sources )
    {
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        unsigned sourceNeuron = sourceIndex._neuron;
        double sourceLb = sourceLayer->getLb( sourceNeuron );
        if ( maxLowerBound < sourceLb )
        {
            indexOfMaxLowerBound = sourceIndex;
            maxLowerBound = sourceLb;
        }
    }

    const Layer *sourceLayer = _layerIndexToLayer[indexOfMaxLowerBound._layer];
    unsigned sourceNeuron = indexOfMaxLowerBound._neuron;
    double sourceLb = sourceLayer->getLb( sourceNeuron );
    double sourceUb = sourceLayer->getUb( sourceNeuron );
    unsigned numberOfCandidates = GlobalConfiguration::PMNR_BBPS_BRANCHING_CANDIDATES;
    for ( unsigned j = 0; j < numberOfCandidates; ++j )
    {
        std::pair<NeuronIndex, double> point( indexOfMaxLowerBound,
                                              sourceLb + ( j + 1 ) * ( sourceUb - sourceLb ) /
                                                             ( numberOfCandidates + 1 ) );
        candidates.append( point );
    }
    const Vector<std::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesForSoftmax( const Layer *layer,
                                                                  unsigned i ) const
{
    // For a Softmax activation, calculate this neuron's source index in the Softmax
    // and sample candidates uniformly in [sourceLb, sourceUb].
    Vector<std::pair<NeuronIndex, double>> candidates;
    List<NeuronIndex> sources = layer->getActivationSources( i );
    NeuronIndex selfIndex( 0, 0 );
    Set<unsigned> handledInputNeurons;
    for ( unsigned j = 0; j < i; ++j )
    {
        for ( const auto &sourceIndex : layer->getActivationSources( j ) )
        {
            if ( !handledInputNeurons.exists( sourceIndex._neuron ) )
            {
                handledInputNeurons.insert( sourceIndex._neuron );
                break;
            }
        }
    }
    for ( const auto &sourceIndex : sources )
    {
        if ( !handledInputNeurons.exists( sourceIndex._neuron ) )
        {
            selfIndex = sourceIndex;
            break;
        }
    }

    const Layer *sourceLayer = _layerIndexToLayer[selfIndex._layer];
    unsigned sourceNeuron = selfIndex._neuron;
    double sourceLb = sourceLayer->getLb( sourceNeuron );
    double sourceUb = sourceLayer->getUb( sourceNeuron );
    unsigned numberOfCandidates = GlobalConfiguration::PMNR_BBPS_BRANCHING_CANDIDATES;
    for ( unsigned j = 0; j < numberOfCandidates; ++j )
    {
        std::pair<NeuronIndex, double> point( selfIndex,
                                              sourceLb + ( j + 1 ) * ( sourceUb - sourceLb ) /
                                                             ( numberOfCandidates + 1 ) );
        candidates.append( point );
    }
    const Vector<std::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

const Vector<std::pair<NeuronIndex, double>>
NetworkLevelReasoner::generateBranchingPointCandidatesForBilinear( const Layer *layer,
                                                                   unsigned i ) const
{
    // For a Bilinear activation, sample candidates uniformly from sources' [sourceLb, sourceUb].
    Vector<std::pair<NeuronIndex, double>> candidates;
    List<NeuronIndex> sources = layer->getActivationSources( i );
    Vector<double> sourceLbs;
    Vector<double> sourceUbs;
    Vector<unsigned> sourceNeurons;
    Vector<unsigned> sourceLayerIndices;
    for ( const auto &sourceIndex : sources )
    {
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        unsigned sourceNeuron = sourceIndex._neuron;
        double sourceLb = sourceLayer->getLb( sourceNeuron );
        double sourceUb = sourceLayer->getUb( sourceNeuron );

        sourceLayerIndices.append( sourceLayer->getLayerIndex() );
        sourceNeurons.append( sourceNeuron );
        sourceLbs.append( sourceLb );
        sourceUbs.append( sourceUb );
    }

    for ( unsigned j = 0; j < sources.size(); ++j )
    {
        unsigned candidatesPerDimension =
            GlobalConfiguration::PMNR_BBPS_BRANCHING_CANDIDATES / sources.size();
        for ( unsigned k = 0; k < candidatesPerDimension; ++k )
        {
            std::pair<NeuronIndex, double> point(
                NeuronIndex( sourceLayerIndices[j], sourceNeurons[j] ),
                sourceLbs[j] +
                    ( k + 1 ) * ( sourceUbs[j] - sourceLbs[j] ) / ( candidatesPerDimension + 1 ) );
            candidates.append( point );
        }
    }
    const Vector<std::pair<NeuronIndex, double>> branchingPointCandidates( candidates );
    return branchingPointCandidates;
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranch(
    NeuronIndex index,
    NeuronIndex sourceIndex,
    const Vector<double> &values,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch,
    unsigned branchCount ) const
{
    ASSERT( symbolicLbPerBranch.size() == branchCount );
    ASSERT( symbolicUbPerBranch.size() == branchCount );
    ASSERT( symbolicLowerBiasPerBranch.size() == branchCount );
    ASSERT( symbolicUpperBiasPerBranch.size() == branchCount );
    ASSERT( values.size() == branchCount + 1 );

    unsigned layerIndex = index._layer;
    Layer *layer = _layerIndexToLayer[layerIndex];
    ASSERT( layer->neuronNonfixed( index._neuron ) );
    Layer::Type type = layer->getLayerType();

    for ( unsigned i = 0; i < branchCount; ++i )
    {
        switch ( type )
        {
        case Layer::RELU:
            calculateSymbolicBoundsPerBranchForRelu( i,
                                                     values[i],
                                                     values[i + 1],
                                                     symbolicLbPerBranch,
                                                     symbolicUbPerBranch,
                                                     symbolicLowerBiasPerBranch,
                                                     symbolicUpperBiasPerBranch );
            break;

        case Layer::ABSOLUTE_VALUE:
            calculateSymbolicBoundsPerBranchForAbsoluteValue( i,
                                                              values[i],
                                                              values[i + 1],
                                                              symbolicLbPerBranch,
                                                              symbolicUbPerBranch,
                                                              symbolicLowerBiasPerBranch,
                                                              symbolicUpperBiasPerBranch );
            break;

        case Layer::SIGN:
            calculateSymbolicBoundsPerBranchForSign( i,
                                                     values[i],
                                                     values[i + 1],
                                                     symbolicLbPerBranch,
                                                     symbolicUbPerBranch,
                                                     symbolicLowerBiasPerBranch,
                                                     symbolicUpperBiasPerBranch );
            break;

        case Layer::ROUND:
            calculateSymbolicBoundsPerBranchForRound( i,
                                                      values[i],
                                                      values[i + 1],
                                                      symbolicLbPerBranch,
                                                      symbolicUbPerBranch,
                                                      symbolicLowerBiasPerBranch,
                                                      symbolicUpperBiasPerBranch );
            break;

        case Layer::SIGMOID:
            calculateSymbolicBoundsPerBranchForSigmoid( i,
                                                        values[i],
                                                        values[i + 1],
                                                        symbolicLbPerBranch,
                                                        symbolicUbPerBranch,
                                                        symbolicLowerBiasPerBranch,
                                                        symbolicUpperBiasPerBranch );
            break;

        case Layer::LEAKY_RELU:
            calculateSymbolicBoundsPerBranchForLeakyRelu( index,
                                                          i,
                                                          values[i],
                                                          values[i + 1],
                                                          symbolicLbPerBranch,
                                                          symbolicUbPerBranch,
                                                          symbolicLowerBiasPerBranch,
                                                          symbolicUpperBiasPerBranch );
            break;

        case Layer::MAX:
            calculateSymbolicBoundsPerBranchForMax( index,
                                                    sourceIndex,
                                                    i,
                                                    values[i],
                                                    values[i + 1],
                                                    symbolicLbPerBranch,
                                                    symbolicUbPerBranch,
                                                    symbolicLowerBiasPerBranch,
                                                    symbolicUpperBiasPerBranch );
            break;

        case Layer::SOFTMAX:
            calculateSymbolicBoundsPerBranchForSoftmax( index,
                                                        sourceIndex,
                                                        i,
                                                        values[i],
                                                        values[i + 1],
                                                        symbolicLbPerBranch,
                                                        symbolicUbPerBranch,
                                                        symbolicLowerBiasPerBranch,
                                                        symbolicUpperBiasPerBranch );
            break;

        case Layer::BILINEAR:
            calculateSymbolicBoundsPerBranchForBilinear( index,
                                                         sourceIndex,
                                                         i,
                                                         values[i],
                                                         values[i + 1],
                                                         symbolicLbPerBranch,
                                                         symbolicUbPerBranch,
                                                         symbolicLowerBiasPerBranch,
                                                         symbolicUpperBiasPerBranch );
            break;

        default:
        {
            printf( "Error! Neuron type %u unsupported\n", type );
            throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
            break;
        }
        }
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForRelu(
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    if ( !FloatUtils::isNegative( sourceLb ) )
    {
        // Phase active
        // Symbolic bound: x_b <= x_f <= x_b
        symbolicUbPerBranch[i] = 1;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = 1;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else if ( !FloatUtils::isPositive( sourceUb ) )
    {
        // Phase inactive
        // Symbolic bound: 0 <= x_f <= 0
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else
    {
        // ReLU not fixed
        // Symbolic upper bound: x_f <= (x_b - l) * u / ( u - l)
        double weight = sourceUb / ( sourceUb - sourceLb );
        symbolicUbPerBranch[i] = weight;
        symbolicUpperBiasPerBranch[i] = -sourceLb * weight;

        // For the lower bound, in general, x_f >= lambda * x_b, where
        // 0 <= lambda <= 1, would be a sound lower bound. We
        // use the heuristic described in section 4.1 of
        // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
        // to set the value of lambda (either 0 or 1 is considered).
        if ( sourceUb > -sourceLb )
        {
            // lambda = 1
            // Symbolic lower bound: x_f >= x_b
            symbolicLbPerBranch[i] = 1;
            symbolicLowerBiasPerBranch[i] = 0;
        }
        else
        {
            // lambda = 1
            // Symbolic lower bound: x_f >= 0
            symbolicLbPerBranch[i] = 0;
            symbolicLowerBiasPerBranch[i] = 0;
        }
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForAbsoluteValue(
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    if ( !FloatUtils::isNegative( sourceLb ) )
    {
        // Phase active
        // Symbolic bound: x_b <= x_f <= x_b
        symbolicUbPerBranch[i] = 1;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = 1;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else if ( !FloatUtils::isPositive( sourceUb ) )
    {
        // Phase inactive
        // Symbolic bound: -x_b <= x_f <= -x_b
        symbolicUbPerBranch[i] = -1;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = -1;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else
    {
        // AbsoluteValue not fixed
        // Naive concretization: 0 <= x_f <= max(-lb, ub)
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = FloatUtils::max( -sourceLb, sourceUb );

        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = 0;
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForSign(
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    if ( !FloatUtils::isNegative( sourceLb ) )
    {
        // Phase active
        // Symbolic bound: 1 <= x_f <= 1
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = 1;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = 1;
    }
    else if ( !FloatUtils::isPositive( sourceUb ) )
    {
        // Phase inactive
        // Symbolic bound: -1 <= x_f <= -1
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = -1;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = -1;
    }
    else
    {
        // Sign not fixed
        // Use the relaxation defined in https://arxiv.org/pdf/2011.02948.pdf
        // Symbolic upper bound: x_f <= -2 / l * x_b + 1
        symbolicUbPerBranch[i] = -2 / sourceLb;
        symbolicUpperBiasPerBranch[i] = 1;

        // Symbolic lower bound: x_f >= (2 / u) * x_b - 1
        symbolicLbPerBranch[i] = 2 / sourceUb;
        symbolicLowerBiasPerBranch[i] = -1;
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForRound(
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    double sourceUbRound = FloatUtils::round( sourceUb );
    double sourceLbRound = FloatUtils::round( sourceLb );

    if ( sourceUbRound == sourceLbRound )
    {
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = sourceUbRound;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = sourceLbRound;
    }
    else
    {
        // Round not fixed
        // Symbolic upper bound: x_f <= x_b + 0.5
        symbolicUbPerBranch[i] = 1;
        symbolicUpperBiasPerBranch[i] = 0.5;

        // Symbolic lower bound: x_f >= x_b - 0.5
        symbolicLbPerBranch[i] = 1;
        symbolicLowerBiasPerBranch[i] = -0.5;
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForSigmoid(
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    double sourceUbSigmoid = SigmoidConstraint::sigmoid( sourceUb );
    double sourceLbSigmoid = SigmoidConstraint::sigmoid( sourceLb );

    if ( sourceUb == sourceLb )
    {
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = sourceUbSigmoid;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = sourceLbSigmoid;
    }
    else
    {
        double lambda = ( sourceUbSigmoid - sourceLbSigmoid ) / ( sourceUb - sourceLb );
        double lambdaPrime = std::min( SigmoidConstraint::sigmoidDerivative( sourceLb ),
                                       SigmoidConstraint::sigmoidDerivative( sourceUb ) );

        // update lower bound
        if ( FloatUtils::isPositive( sourceLb ) )
        {
            symbolicLbPerBranch[i] = lambda;
            symbolicLowerBiasPerBranch[i] = sourceLbSigmoid - lambda * sourceLb;
        }
        else
        {
            symbolicLbPerBranch[i] = lambdaPrime;
            symbolicLowerBiasPerBranch[i] = sourceLbSigmoid - lambdaPrime * sourceLb;
        }

        // update upper bound
        if ( !FloatUtils::isPositive( sourceUb ) )
        {
            symbolicUbPerBranch[i] = lambda;
            symbolicUpperBiasPerBranch[i] = sourceUbSigmoid - lambda * sourceUb;
        }
        else
        {
            symbolicUbPerBranch[i] = lambdaPrime;
            symbolicUpperBiasPerBranch[i] = sourceUbSigmoid - lambdaPrime * sourceUb;
        }
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForLeakyRelu(
    NeuronIndex index,
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    double slope = _layerIndexToLayer[index._layer]->getAlpha();
    if ( !FloatUtils::isNegative( sourceLb ) )
    {
        // Phase active
        // Symbolic bound: x_b <= x_f <= x_b
        symbolicUbPerBranch[i] = 1;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = 1;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else if ( !FloatUtils::isPositive( sourceUb ) )
    {
        // Phase inactive
        // Symbolic bound: slope * x_b <= x_f <= slope * x_b
        symbolicUbPerBranch[i] = slope;
        symbolicUpperBiasPerBranch[i] = 0;
        symbolicLbPerBranch[i] = slope;
        symbolicLowerBiasPerBranch[i] = 0;
    }
    else
    {
        // LeakyReLU not fixed
        // Symbolic upper bound: x_f <= (x_b - l) * u / ( u - l)
        double width = sourceUb - sourceLb;
        double weight = ( sourceUb - slope * sourceLb ) / width;

        if ( slope <= 1 )
        {
            symbolicUbPerBranch[i] = weight;
            symbolicUpperBiasPerBranch[i] = ( ( slope - 1 ) * sourceUb * sourceLb ) / width;

            // For the lower bound, in general, x_f >= lambda * x_b, where
            // 0 <= lambda <= 1, would be a sound lower bound. We
            // use the heuristic described in section 4.1 of
            // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
            // to set the value of lambda (either 0 or 1 is considered).
            if ( sourceUb > sourceLb )
            {
                // lambda = 1
                // Symbolic lower bound: x_f >= x_b
                symbolicLbPerBranch[i] = 1;
                symbolicLowerBiasPerBranch[i] = 0;
            }
            else
            {
                // lambda = 1
                // Symbolic lower bound: x_f >= 0
                // Concrete lower bound: x_f >= 0
                symbolicLbPerBranch[i] = slope;
                symbolicLowerBiasPerBranch[i] = 0;
            }
        }
        else
        {
            symbolicLbPerBranch[i] = weight;
            symbolicLowerBiasPerBranch[i] = ( ( slope - 1 ) * sourceUb * sourceLb ) / width;

            if ( sourceUb > sourceLb )
            {
                symbolicUbPerBranch[i] = 1;
                symbolicUpperBiasPerBranch[i] = 0;
            }
            else
            {
                symbolicUbPerBranch[i] = slope;
                symbolicLowerBiasPerBranch[i] = 0;
            }
        }
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForMax(
    NeuronIndex index,
    NeuronIndex chosenSourceIndex,
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    unsigned layerIndex = index._layer;
    unsigned neuron = index._neuron;
    Layer *layer = _layerIndexToLayer[layerIndex];
    List<NeuronIndex> sources = layer->getActivationSources( neuron );

    NeuronIndex indexOfMaxLowerBound = *( sources.begin() );
    double maxLowerBound = FloatUtils::negativeInfinity();
    double maxUpperBound = FloatUtils::negativeInfinity();

    Map<NeuronIndex, double> sourceLbs;
    Map<NeuronIndex, double> sourceUbs;
    for ( const auto &sourceIndex : sources )
    {
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        unsigned sourceNeuron = sourceIndex._neuron;
        double currentLb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getLb( sourceNeuron ) : sourceLb;
        double currentUb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getUb( sourceNeuron ) : sourceUb;
        sourceLbs[sourceIndex] = currentLb;
        sourceUbs[sourceIndex] = currentUb;

        if ( maxLowerBound < currentLb )
        {
            indexOfMaxLowerBound = sourceIndex;
            maxLowerBound = currentLb;
        }
        if ( maxUpperBound < currentUb )
        {
            maxUpperBound = currentUb;
        }
    }

    // The phase is fixed if the lower-bound of a source variable x_b is
    // larger than the upper-bounds of the other source variables.
    bool phaseFixed = true;
    for ( const auto &sourceIndex : sources )
    {
        if ( sourceIndex != indexOfMaxLowerBound &&
             FloatUtils::gt( sourceUbs[sourceIndex], maxLowerBound ) )
        {
            phaseFixed = false;
            break;
        }
    }

    if ( phaseFixed )
    {
        // Phase fixed
        // Symbolic bound: x_b <= x_f <= x_b
        // Concretized bound (if chosenSourceIndex != indexOfMaxLowerBound): x_b.lb <= x_f <=
        // x_b.ub.
        if ( chosenSourceIndex != indexOfMaxLowerBound )
        {
            symbolicLbPerBranch[i] = 0;
            symbolicLowerBiasPerBranch[i] = sourceLbs[indexOfMaxLowerBound];
            symbolicUbPerBranch[i] = 0;
            symbolicUpperBiasPerBranch[i] = sourceUbs[indexOfMaxLowerBound];
        }
        else
        {
            symbolicLbPerBranch[i] = 1;
            symbolicLowerBiasPerBranch[i] = 0;
            symbolicUbPerBranch[i] = 1;
            symbolicUpperBiasPerBranch[i] = 0;
        }
    }
    else
    {
        // MaxPool not fixed
        // Symbolic bounds: x_b <= x_f <= maxUpperBound
        // Concretized bound (if chosenSourceIndex != indexOfMaxLowerBound): x_b.lb <= x_f <=
        // maxUpperBound.
        if ( chosenSourceIndex != indexOfMaxLowerBound )
        {
            symbolicLbPerBranch[i] = 0;
            symbolicLowerBiasPerBranch[i] = sourceLbs[indexOfMaxLowerBound];
            symbolicUbPerBranch[i] = 0;
            symbolicUpperBiasPerBranch[i] = maxUpperBound;
        }
        else
        {
            symbolicLbPerBranch[i] = 1;
            symbolicLowerBiasPerBranch[i] = 0;
            symbolicUbPerBranch[i] = 0;
            symbolicUpperBiasPerBranch[i] = maxUpperBound;
        }
    }
}

void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForSoftmax(
    NeuronIndex index,
    NeuronIndex chosenSourceIndex,
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    unsigned layerIndex = index._layer;
    unsigned neuron = index._neuron;
    Layer *layer = _layerIndexToLayer[layerIndex];
    List<NeuronIndex> sources = layer->getActivationSources( neuron );
    Vector<double> sourceLbs;
    Vector<double> sourceUbs;
    Vector<double> sourceMids;
    Vector<double> targetLbs;
    Vector<double> targetUbs;
    for ( const auto &sourceIndex : sources )
    {
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        unsigned sourceNeuron = sourceIndex._neuron;
        double currentLb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getLb( sourceNeuron ) : sourceLb;
        double currentUb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getUb( sourceNeuron ) : sourceUb;
        sourceLbs.append( currentLb - GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
        sourceUbs.append( currentUb + GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
        sourceMids.append( ( currentLb + currentUb ) / 2 );
        targetLbs.append( layer->getLb( neuron ) );
        targetUbs.append( layer->getUb( neuron ) );
    }

    unsigned selfIndex = 0;
    Set<unsigned> handledInputNeurons;
    for ( unsigned i = 0; i < neuron; ++i )
    {
        for ( const auto &sourceIndex : layer->getActivationSources( i ) )
        {
            if ( !handledInputNeurons.exists( sourceIndex._neuron ) )
            {
                handledInputNeurons.insert( sourceIndex._neuron );
                break;
            }
        }
    }
    for ( const auto &sourceIndex : sources )
    {
        if ( handledInputNeurons.exists( sourceIndex._neuron ) )
            ++selfIndex;
        else
        {
            break;
        }
    }

    double lb = std::max( Layer::linearLowerBound( sourceLbs, sourceUbs, selfIndex ),
                          layer->getLb( neuron ) );
    double ub = std::min( Layer::linearUpperBound( sourceLbs, sourceUbs, selfIndex ),
                          layer->getUb( neuron ) );
    targetLbs[selfIndex] = lb;
    targetUbs[selfIndex] = ub;

    if ( FloatUtils::areEqual( lb, ub ) )
    {
        symbolicUbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = ub;
        symbolicLbPerBranch[i] = 0;
        symbolicLowerBiasPerBranch[i] = lb;
    }
    else
    {
        // Compute Softmax symbolic bound. Neurons other than given source neuron are concretized.
        if ( Options::get()->getSoftmaxBoundType() == SoftmaxBoundType::LOG_SUM_EXP_DECOMPOSITION )
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
                symbolicLowerBiasPerBranch[i] =
                    Layer::LSELowerBound( sourceMids, sourceLbs, sourceUbs, selfIndex );
                for ( const auto &sourceIndex : sources )
                {
                    const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                    double dldj = Layer::dLSELowerBound(
                        sourceMids, sourceLbs, sourceUbs, selfIndex, inputIndex );
                    if ( sourceIndex != chosenSourceIndex )
                    {
                        double concretizedLowerBias =
                            dldj > 0 ? dldj * sourceLayer->getLb( sourceIndex._neuron )
                                     : dldj * sourceLayer->getUb( sourceIndex._neuron );
                        symbolicLowerBiasPerBranch[i] += concretizedLowerBias;
                    }
                    else
                    {
                        symbolicLbPerBranch[i] = dldj;
                    }
                    symbolicLowerBiasPerBranch[i] -= dldj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
            else
            {
                symbolicLowerBiasPerBranch[i] =
                    Layer::LSELowerBound2( sourceMids, sourceLbs, sourceUbs, selfIndex );
                for ( const auto &sourceIndex : sources )
                {
                    const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                    double dldj = Layer::dLSELowerBound2(
                        sourceMids, sourceLbs, sourceUbs, selfIndex, inputIndex );
                    if ( sourceIndex != chosenSourceIndex )
                    {
                        double concretizedLowerBias =
                            dldj > 0 ? dldj * sourceLayer->getLb( sourceIndex._neuron )
                                     : dldj * sourceLayer->getUb( sourceIndex._neuron );
                        symbolicLowerBiasPerBranch[i] += concretizedLowerBias;
                    }
                    else
                    {
                        symbolicLbPerBranch[i] = dldj;
                    }
                    symbolicLowerBiasPerBranch[i] -= dldj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }

            symbolicUpperBiasPerBranch[i] =
                Layer::LSEUpperBound( sourceMids, targetLbs, targetUbs, selfIndex );
            inputIndex = 0;
            for ( const auto &sourceIndex : sources )
            {
                const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                double dudj = Layer::dLSEUpperbound(
                    sourceMids, targetLbs, targetUbs, selfIndex, inputIndex );
                if ( sourceIndex != chosenSourceIndex )
                {
                    double concretizedUpperBias =
                        dudj > 0 ? dudj * sourceLayer->getUb( sourceIndex._neuron )
                                 : dudj * sourceLayer->getLb( sourceIndex._neuron );
                    symbolicUpperBiasPerBranch[i] += concretizedUpperBias;
                }
                else
                {
                    symbolicUbPerBranch[i] = dudj;
                }
                symbolicUpperBiasPerBranch[i] -= dudj * sourceMids[inputIndex];
                ++inputIndex;
            }
        }
        else if ( Options::get()->getSoftmaxBoundType() ==
                  SoftmaxBoundType::EXPONENTIAL_RECIPROCAL_DECOMPOSITION )
        {
            symbolicLowerBiasPerBranch[i] =
                Layer::ERLowerBound( sourceMids, sourceLbs, sourceUbs, selfIndex );
            unsigned inputIndex = 0;
            for ( const auto &sourceIndex : sources )
            {
                const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                double dldj =
                    Layer::dERLowerBound( sourceMids, sourceLbs, sourceUbs, selfIndex, inputIndex );
                if ( sourceIndex != chosenSourceIndex )
                {
                    double concretizedLowerBias =
                        dldj > 0 ? dldj * sourceLayer->getLb( sourceIndex._neuron )
                                 : dldj * sourceLayer->getUb( sourceIndex._neuron );
                    symbolicLowerBiasPerBranch[i] += concretizedLowerBias;
                }
                else
                {
                    symbolicLbPerBranch[i] = dldj;
                }
                symbolicLowerBiasPerBranch[i] -= dldj * sourceMids[inputIndex];
                ++inputIndex;
            }

            symbolicUpperBiasPerBranch[i] =
                Layer::ERUpperBound( sourceMids, targetLbs, targetUbs, selfIndex );
            inputIndex = 0;
            for ( const auto &sourceIndex : sources )
            {
                const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
                double dudj =
                    Layer::dERUpperBound( sourceMids, targetLbs, targetUbs, selfIndex, inputIndex );
                if ( sourceIndex != chosenSourceIndex )
                {
                    double concretizedUpperBias =
                        dudj > 0 ? dudj * sourceLayer->getUb( sourceIndex._neuron )
                                 : dudj * sourceLayer->getLb( sourceIndex._neuron );
                    symbolicUpperBiasPerBranch[i] += concretizedUpperBias;
                }
                else
                {
                    symbolicUbPerBranch[i] = dudj;
                }
                symbolicUpperBiasPerBranch[i] -= dudj * sourceMids[inputIndex];
                ++inputIndex;
            }
        }
    }
}
void NetworkLevelReasoner::calculateSymbolicBoundsPerBranchForBilinear(
    NeuronIndex index,
    NeuronIndex chosenSourceIndex,
    unsigned i,
    double sourceLb,
    double sourceUb,
    Vector<double> &symbolicLbPerBranch,
    Vector<double> &symbolicUbPerBranch,
    Vector<double> &symbolicLowerBiasPerBranch,
    Vector<double> &symbolicUpperBiasPerBranch ) const
{
    unsigned layerIndex = index._layer;
    unsigned neuron = index._neuron;
    Layer *layer = _layerIndexToLayer[layerIndex];
    List<NeuronIndex> sources = layer->getActivationSources( neuron );

    Vector<double> sourceLbs;
    Vector<double> sourceUbs;
    Vector<double> sourceValues;
    Vector<NeuronIndex> sourceNeurons;
    Vector<unsigned> sourceLayerSizes;
    Vector<const Layer *> sourceLayers;
    bool allConstant = true;
    for ( const auto &sourceIndex : sources )
    {
        const Layer *sourceLayer = _layerIndexToLayer[sourceIndex._layer];
        unsigned sourceNeuron = sourceIndex._neuron;
        double currentLb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getLb( sourceNeuron ) : sourceLb;
        double currentUb =
            sourceIndex != chosenSourceIndex ? sourceLayer->getUb( sourceNeuron ) : sourceUb;

        sourceLayers.append( sourceLayer );
        sourceNeurons.append( sourceIndex );
        sourceLbs.append( currentLb );
        sourceUbs.append( currentUb );

        if ( !sourceLayer->neuronEliminated( sourceNeuron ) )
        {
            allConstant = false;
        }
        else
        {
            double sourceValue = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
            sourceValues.append( sourceValue );
        }
    }

    if ( allConstant )
    {
        // If the both source neurons have been eliminated, this neuron is constant
        symbolicUbPerBranch[i] = 0;
        symbolicLbPerBranch[i] = 0;
        symbolicUpperBiasPerBranch[i] = sourceValues[0] * sourceValues[1];
        symbolicLowerBiasPerBranch[i] = sourceValues[0] * sourceValues[1];
    }
    else
    {
        // Symbolic lower bound:
        // out >= alpha * x + beta * y + gamma
        // where alpha = lb_y, beta = lb_x, gamma = -lb_x * lb_y.

        // Symbolic upper bound:
        // out <= alpha * x + beta * y + gamma
        // where alpha = ub_y, beta = lb_x, gamma = -lb_x * ub_y.

        // Neuron other than given source neuron is concretized.

        double aLower = sourceLbs[1];
        double aUpper = sourceUbs[1];
        double bLower = sourceLbs[0];
        double bUpper = sourceLbs[0];
        if ( sourceNeurons[0] != chosenSourceIndex )
        {
            double concretizedLowerBias =
                aLower > 0 ? aLower * sourceLayers[0]->getLb( sourceNeurons[0]._neuron )
                           : aLower * sourceLayers[0]->getUb( sourceNeurons[0]._neuron );
            double concretizedUpperBias =
                aUpper > 0 ? aUpper * sourceLayers[0]->getUb( sourceNeurons[0]._neuron )
                           : aUpper * sourceLayers[0]->getLb( sourceNeurons[0]._neuron );
            symbolicLbPerBranch[i] = bLower;
            symbolicUbPerBranch[i] = bUpper;
            symbolicLowerBiasPerBranch[i] += concretizedLowerBias;
            symbolicUpperBiasPerBranch[i] += concretizedUpperBias;
        }
        else
        {
            double concretizedLowerBias =
                bLower > 0 ? bLower * sourceLayers[1]->getLb( sourceNeurons[1]._neuron )
                           : bLower * sourceLayers[1]->getUb( sourceNeurons[1]._neuron );
            double concretizedUpperBias =
                bUpper > 0 ? bUpper * sourceLayers[1]->getUb( sourceNeurons[1]._neuron )
                           : bUpper * sourceLayers[1]->getLb( sourceNeurons[1]._neuron );
            symbolicLbPerBranch[i] = aLower;
            symbolicUbPerBranch[i] = aUpper;
            symbolicUpperBiasPerBranch[i] += concretizedUpperBias;
            symbolicLowerBiasPerBranch[i] += concretizedLowerBias;
        }
        symbolicLowerBiasPerBranch[i] += -sourceLbs[0] * sourceLbs[1];
        symbolicUpperBiasPerBranch[i] += -sourceLbs[0] * sourceUbs[1];
    }
}

double
NetworkLevelReasoner::calculateTighteningLoss( const Vector<double> &values,
                                               const Vector<double> &symbolicLbPerBranch,
                                               const Vector<double> &symbolicUbPerBranch,
                                               const Vector<double> &symbolicLowerBiasPerBranch,
                                               const Vector<double> &symbolicUpperBiasPerBranch,
                                               unsigned branchCount ) const
{
    ASSERT( symbolicLbPerBranch.size() == branchCount );
    ASSERT( symbolicUbPerBranch.size() == branchCount );
    ASSERT( symbolicLowerBiasPerBranch.size() == branchCount );
    ASSERT( symbolicUpperBiasPerBranch.size() == branchCount );
    ASSERT( values.size() == branchCount + 1 );

    double score = 0;
    for ( unsigned i = 0; i < branchCount; ++i )
    {
        // Given branch #i symbolic bounds of x_f >= a_l x_b + b_l, x_f <= a_u x_b + b_u, x_b in
        // [l_i, u_i], calculate integral of ( a_u x_b + b_u ) - ( a_l x_b + b_l ) in [l_i, u_i].
        score += symbolicUbPerBranch[i] *
                 ( std::pow( values[i + 1], 2 ) - std::pow( values[i], 2 ) ) / 2;
        score += symbolicUpperBiasPerBranch[i] * ( values[i + 1] - values[i] );
        score -= symbolicLbPerBranch[i] *
                 ( std::pow( values[i + 1], 2 ) - std::pow( values[i], 2 ) ) / 2;
        score -= symbolicLowerBiasPerBranch[i] * ( values[i + 1] - values[i] );
    }
    return score;
}

const Map<unsigned, Vector<double>>
NetworkLevelReasoner::getParametersForLayers( const Vector<double> &coeffs ) const
{
    ASSERT( coeffs.size() == getNumberOfParameters() );
    unsigned index = 0;
    Map<unsigned, Vector<double>> layerIndicesToParameters;
    for ( const auto &pair : _layerIndexToLayer )
    {
        unsigned layerIndex = pair.first;
        Layer *layer = pair.second;
        unsigned coeffsCount = getNumberOfParametersPerType( layer->getLayerType() );
        Vector<double> currentCoeffs( coeffsCount );
        for ( unsigned i = 0; i < coeffsCount; ++i )
        {
            currentCoeffs[i] = coeffs[index + i];
        }
        layerIndicesToParameters.insert( layerIndex, currentCoeffs );
        index += coeffsCount;
    }
    const Map<unsigned, Vector<double>> parametersForLayers( layerIndicesToParameters );
    return parametersForLayers;
}

unsigned NetworkLevelReasoner::getNumberOfParameters() const
{
    unsigned num = 0;
    for ( const auto &pair : _layerIndexToLayer )
    {
        Layer *layer = pair.second;
        num += getNumberOfParametersPerType( layer->getLayerType() );
    }
    return num;
}

unsigned NetworkLevelReasoner::getNumberOfParametersPerType( Layer::Type t ) const
{
    if ( t == Layer::RELU || t == Layer::LEAKY_RELU )
        return 1;

    if ( t == Layer::SIGN || t == Layer::BILINEAR )
        return 2;

    return 0;
}

bool NetworkLevelReasoner::supportsInvpropBranching( Layer::Type type ) const
{
    // When using BBPS heuristic, all implemented activations could be branched before INVPROP.
    if ( Options::get()->getMILPSolverBoundTighteningType() ==
         MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_PMNR )
    {
        return type == Layer::RELU || type == Layer::LEAKY_RELU || type == Layer::SIGN ||
               type == Layer::ABSOLUTE_VALUE || type == Layer::MAX || type == Layer::ROUND ||
               type == Layer::SIGMOID || type == Layer::SOFTMAX || type == Layer::BILINEAR;
    }
    return false;
}

const Vector<unsigned> NetworkLevelReasoner::getLayersWithNonfixedNeurons() const
{
    Vector<unsigned> layerWithNonfixedNeurons = Vector<unsigned>( {} );
    for ( const auto &pair : _layerIndexToLayer )
    {
        if ( !pair.second->getNonfixedNeurons().empty() )
        {
            layerWithNonfixedNeurons.append( pair.first );
        }
    }
    const Vector<unsigned> layerList = Vector<unsigned>( layerWithNonfixedNeurons );
    return layerList;
}

void NetworkLevelReasoner::mergeWSLayers( unsigned secondLayerIndex,
                                          Map<unsigned, LinearExpression> &eliminatedNeurons )
{
    Layer *secondLayer = _layerIndexToLayer[secondLayerIndex];
    unsigned firstLayerIndex = secondLayer->getSourceLayers().begin()->first;
    Layer *firstLayer = _layerIndexToLayer[firstLayerIndex];
    unsigned lastLayerIndex = _layerIndexToLayer.size() - 1;

    // Iterate over all inputs to the first layer
    for ( const auto &pair : firstLayer->getSourceLayers() )
    {
        unsigned previousToFirstLayerIndex = pair.first;
        const Layer *inputLayerToFirst = _layerIndexToLayer[previousToFirstLayerIndex];

        unsigned inputDimension = inputLayerToFirst->getSize();
        unsigned middleDimension = firstLayer->getSize();
        unsigned outputDimension = secondLayer->getSize();

        // Compute new weights
        const double *firstLayerMatrix = firstLayer->getWeightMatrix( previousToFirstLayerIndex );
        const double *secondLayerMatrix = secondLayer->getWeightMatrix( firstLayerIndex );
        double *newWeightMatrix = multiplyWeights(
            firstLayerMatrix, secondLayerMatrix, inputDimension, middleDimension, outputDimension );
        // Update bias for second layer
        for ( unsigned targetNeuron = 0; targetNeuron < secondLayer->getSize(); ++targetNeuron )
        {
            double newBias = secondLayer->getBias( targetNeuron );
            for ( unsigned sourceNeuron = 0; sourceNeuron < firstLayer->getSize(); ++sourceNeuron )
            {
                newBias +=
                    ( firstLayer->getBias( sourceNeuron ) *
                      secondLayer->getWeight( firstLayerIndex, sourceNeuron, targetNeuron ) );
            }

            secondLayer->setBias( targetNeuron, newBias );
        }

        // Adjust the sources of the second layer
        secondLayer->addSourceLayer( previousToFirstLayerIndex, inputLayerToFirst->getSize() );
        for ( unsigned sourceNeuron = 0; sourceNeuron < inputDimension; ++sourceNeuron )
        {
            for ( unsigned targetNeuron = 0; targetNeuron < outputDimension; ++targetNeuron )
            {
                double weight = newWeightMatrix[sourceNeuron * outputDimension + targetNeuron];
                secondLayer->setWeight(
                    previousToFirstLayerIndex, sourceNeuron, targetNeuron, weight );
            }
        }
        delete[] newWeightMatrix;
    }

    // Remove the first layer from second layer's sources
    secondLayer->removeSourceLayer( firstLayerIndex );

    generateLinearExpressionForWeightedSumLayer( eliminatedNeurons, firstLayer );

    // Finally, remove the first layer from the map and delete it
    _layerIndexToLayer.erase( firstLayerIndex );
    delete firstLayer;

    // Adjust the indices of all layers starting from secondLayerIndex
    // and higher
    for ( unsigned i = secondLayerIndex; i <= lastLayerIndex; ++i )
        reduceLayerIndex( i, secondLayerIndex );
}

double *NetworkLevelReasoner::multiplyWeights( const double *firstMatrix,
                                               const double *secondMatrix,
                                               unsigned inputDimension,
                                               unsigned middleDimension,
                                               unsigned outputDimension )
{
    double *newMatrix = new double[inputDimension * outputDimension];
    std::fill_n( newMatrix, inputDimension * outputDimension, 0 );
    matrixMultiplication(
        firstMatrix, secondMatrix, newMatrix, inputDimension, middleDimension, outputDimension );
    return newMatrix;
}

void NetworkLevelReasoner::reduceLayerIndex( unsigned layer, unsigned startIndex )
{
    // update Layer-level maps
    _layerIndexToLayer[layer]->reduceIndexFromAllMaps( startIndex );
    _layerIndexToLayer[layer]->reduceIndexAfterMerge( startIndex );

    // Update the mapping in the NLR
    _layerIndexToLayer[layer - 1] = _layerIndexToLayer[layer];
    _layerIndexToLayer.erase( layer );
}

void NetworkLevelReasoner::dumpBounds()
{
    obtainCurrentBounds();

    for ( const auto &layer : _layerIndexToLayer )
        layer.second->dumpBounds();
}

unsigned NetworkLevelReasoner::getMaxLayerSize() const
{
    unsigned maxSize = 0;
    for ( const auto &layer : _layerIndexToLayer )
    {
        unsigned currentSize = layer.second->getSize();
        if ( currentSize > maxSize )
            maxSize = currentSize;
    }
    ASSERT( maxSize > 0 );
    return maxSize;
}

const Map<unsigned, Layer *> &NetworkLevelReasoner::getLayerIndexToLayer() const
{
    return _layerIndexToLayer;
}

} // namespace NLR
