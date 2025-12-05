/*********************                                                        */
/*! \file NetworkLevelReasoner.h
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

#ifndef __NetworkLevelReasoner_h__
#define __NetworkLevelReasoner_h__

#include "DeepPolyAnalysis.h"
#include "ITableau.h"
#include "Layer.h"
#include "LayerOwner.h"
#include "Map.h"
#include "MatrixMultiplication.h"
#include "NeuronIndex.h"
#include "PiecewiseLinearFunctionType.h"
#include "PolygonalTightening.h"
#include "Tightening.h"
#include "Vector.h"

#include <memory>

namespace NLR {

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner : public LayerOwner
{
public:
    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    static bool functionTypeSupported( PiecewiseLinearFunctionType type );

    /*
      Populate the NLR by specifying the network's topology.
    */
    void addLayer( unsigned layerIndex, Layer::Type type, unsigned layerSize );
    void addLayerDependency( unsigned sourceLayer, unsigned targetLayer );
    void removeLayerDependency( unsigned sourceLayer, unsigned targetLayer );
    void computeSuccessorLayers();
    void setWeight( unsigned sourceLayer,
                    unsigned sourceNeuron,
                    unsigned targetLayer,
                    unsigned targetNeuron,
                    double weight );
    void setBias( unsigned layer, unsigned neuron, double bias );
    void addActivationSource( unsigned sourceLayer,
                              unsigned sourceNeuron,
                              unsigned targetLeyer,
                              unsigned targetNeuron );

    unsigned getNumberOfLayers() const;
    const Layer *getLayer( unsigned index ) const;
    Layer *getLayer( unsigned index );

    /*
      Bind neurons in the NLR to the Tableau variables that represent them.
    */
    void setNeuronVariable( NeuronIndex index, unsigned variable );

    /*
      Perform an evaluation of the network for a specific input.
    */
    void evaluate( double *input, double *output );

    /*
      Perform an evaluation of the network for the current input variable
      assignment and store the resulting variable assignment in the assignment.
    */
    void concretizeInputAssignment( Map<unsigned, double> &assignment );

    /*
      Perform a simulation of the network for a specific input
    */
    void simulate( Vector<Vector<double>> *input );

    /*
      Bound propagation methods:

        - obtainCurrentBounds: make the NLR obtain the current bounds
          on all variables from the tableau.

        - Interval arithmetic: compute the bounds of a layer's neurons
          based on the concrete bounds of the previous layer.

        - Symbolic: for each neuron in the network, we compute lower
          and upper bounds on the lower and upper bounds of the
          neuron. This bounds are expressed as linear combinations of
          the input neurons. Sometimes these bounds let us simplify
          expressions and obtain tighter bounds (e.g., if the upper
          bound on the upper bound of a ReLU node is negative, that
          ReLU is inactive and its output can be set to 0.

        - Parametrised Symbolic: For certain activation functions, there
          is a continuum of valid symbolic bounds. We receive a map of
          coefficients in range [0, 1] for every layer index, then compute
          the parameterised symbolic bounds (or default to regular
          symbolic bounds if parameterised bounds not implemented).

        - DeepPoly: For every neuron in the network, calculate symbolic
          bounds in term of its predecessor neurons, then perform
          backsubstitution up to the input layer and concretize.

        - Parameterised DeepPoly: For certain activation functions, there
          is a continuum of valid symbolic bounds. We receive a map of
          coefficients in range [0, 1] for every layer index, then compute
          the DeepPoly bounds via backsubstitution and concretization. For
          each layer, a symbolic bound in terms its highest predecessor layers
          is stored in the predessorSymbolic maps. Symbolic bounds for the last
          layer in terms of every layer are stored in the outputLayer maps.

        - LP Relaxation: invoking an LP solver on a series of LP
          relaxations of the problem we're trying to solve, and
          optimizing the lower and upper bounds of each of the
          varibales.

        - receiveTighterBound: this is a callback from the layer
          objects, through which they report tighter bounds.

        - getConstraintTightenings: this is the function that an
          external user calls in order to collect the tighter bounds
          discovered by the NLR.

        - receivePolygonalTightening: this is a callback from the layer
          objects, through which they report polygonal bounds.

        - getPolygonalTightenings: this is the function that an
          external user calls in order to collect the polygonal bounds
          discovered by the NLR.

        - receiveInfeasibleBranches: this is a callback from the layer
          objects, through which they report infeasible branches combinations.

        - getPolygonalTightenings: this is the function that an
          external user calls in order to collect the infeasible branches combinations
          discovered by the NLR.
    */

    void setTableau( const ITableau *tableau );
    const ITableau *getTableau() const;

    void obtainCurrentBounds( const Query &inputQuery );
    void obtainCurrentBounds();
    void intervalArithmeticBoundPropagation();
    void symbolicBoundPropagation();
    void parameterisedSymbolicBoundPropagation( const Vector<double> &coeffs );
    void deepPolyPropagation();
    void parameterisedDeepPoly( bool storeSymbolicBounds = false,
                                const Vector<double> &coeffs = Vector<double>( {} ) );
    void lpRelaxationPropagation();
    void LPTighteningForOneLayer( unsigned targetIndex );
    void MILPPropagation();
    void MILPTighteningForOneLayer( unsigned targetIndex );
    void iterativePropagation();

    void receiveTighterBound( Tightening tightening );
    void getConstraintTightenings( List<Tightening> &tightenings );
    void clearConstraintTightenings();

    void receivePolygonalTightening( PolygonalTightening &polygonalTightening );
    void getPolygonalTightenings( List<PolygonalTightening> &polygonalTightenings );
    void clearPolygonalTightenings();

    void receiveInfeasibleBranches( Map<NeuronIndex, unsigned> &neuronToBranchIndex );
    void getInfeasibleBranches( List<Map<NeuronIndex, unsigned>> &infeasibleBranches );
    void clearInfeasibleBranches();

    // Get total number of optimizable parameters for parameterised SBT relaxation.
    unsigned getNumberOfParameters() const;

    /*
      For debugging purposes: dump the network topology
    */
    void dumpTopology( bool dumpLayerDetails = true ) const;

    /*
      Duplicate the reasoner
    */
    void storeIntoOther( NetworkLevelReasoner &other ) const;

    /*
      Methods that are typically invoked by the preprocessor, to
      inform us of changes in variable indices or if a variable has
      been eliminated
    */
    void eliminateVariable( unsigned variable, double value );
    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables );

    /*
      The various piecewise-linear constraints, sorted in topological
      order. The sorting is done externally.
    */
    List<PiecewiseLinearConstraint *> getConstraintsInTopologicalOrder();
    void addConstraintInTopologicalOrder( PiecewiseLinearConstraint *constraint );
    void removeConstraintFromTopologicalOrder( PiecewiseLinearConstraint *constraint );

    /*
      Add an ecoding of all the affine layers as equations in the given Query
    */
    void encodeAffineLayers( Query &inputQuery );

    /*
      Generate an input query from this NLR, according to the
      discovered network topology
    */
    void generateQuery( Query &query );

    /*
      Given a ReLU Constraint, get the previous layer bias
      for the BaBSR Heuristic
    */
    double getPreviousBias( const ReluConstraint *reluConstraint ) const;

    // Get symbolic bounds for the last layer in term of given layer.
    Vector<double> getOutputSymbolicLb( unsigned layerIndex );
    Vector<double> getOutputSymbolicUb( unsigned layerIndex );
    Vector<double> getOutputSymbolicLowerBias( unsigned layerIndex );
    Vector<double> getOutputSymbolicUpperBias( unsigned layerIndex );

    // Get symbolic bounds of given layer in terms of its predecessor.
    Vector<double> getPredecessorSymbolicLb( unsigned layerIndex );
    Vector<double> getPredecessorSymbolicUb( unsigned layerIndex );
    Vector<double> getPredecessorSymbolicLowerBias( unsigned layerIndex );
    Vector<double> getPredecessorSymbolicUpperBias( unsigned layerIndex );

    /*
       Get the symbolic bounds in term of predecessor layer given branch of given neuron.
    */
    Vector<double> getSymbolicLbPerBranch( NeuronIndex index );
    Vector<double> getSymbolicUbPerBranch( NeuronIndex index );
    Vector<double> getSymbolicLowerBiasPerBranch( NeuronIndex index );
    Vector<double> getSymbolicUpperBiasPerBranch( NeuronIndex index );

    /*
       Get the BBPS branching point of given neuron: Map containing the
       branching point for every predecessor neuron.
    */
    std::pair<NeuronIndex, double> getBBPSBranchingPoint( NeuronIndex index );

    // Get PMNR neuron selection heuristic score for given neuron.
    double getPMNRScore( NeuronIndex index );

    /*
      Finds logically consecutive WS layers and merges them, in order
      to reduce the total number of layers and variables in the
      network
    */
    unsigned mergeConsecutiveWSLayers( const Map<unsigned, double> &lowerBounds,
                                       const Map<unsigned, double> &upperBounds,
                                       const Set<unsigned> &varsInUnhandledConstraints,
                                       Map<unsigned, LinearExpression> &eliminatedNeurons );

    /*
      Print the bounds of variables layer by layer
    */
    void dumpBounds();

    /*
      Get the size of the widest layer
    */
    unsigned getMaxLayerSize() const;

    const Map<unsigned, Layer *> &getLayerIndexToLayer() const;

private:
    Map<unsigned, Layer *> _layerIndexToLayer;
    const ITableau *_tableau;

    // Tightenings and Polyognal Tightenings discovered by the various layers
    List<Tightening> _boundTightenings;
    List<PolygonalTightening> _polygonalBoundTightenings;

    List<Map<NeuronIndex, unsigned>> _infeasibleBranches;

    std::unique_ptr<DeepPolyAnalysis> _deepPolyAnalysis;

    List<PiecewiseLinearConstraint *> _constraintsInTopologicalOrder;

    Map<unsigned, Vector<double>> _predecessorSymbolicLb;
    Map<unsigned, Vector<double>> _predecessorSymbolicUb;
    Map<unsigned, Vector<double>> _predecessorSymbolicLowerBias;
    Map<unsigned, Vector<double>> _predecessorSymbolicUpperBias;

    Map<unsigned, Vector<double>> _outputSymbolicLb;
    Map<unsigned, Vector<double>> _outputSymbolicUb;
    Map<unsigned, Vector<double>> _outputSymbolicLowerBias;
    Map<unsigned, Vector<double>> _outputSymbolicUpperBias;

    Map<NeuronIndex, double> _neuronToPMNRScores;

    Map<NeuronIndex, std::pair<NeuronIndex, double>> _neuronToBBPSBranchingPoints;
    Map<NeuronIndex, Vector<double>> _neuronToSymbolicLbPerBranch;
    Map<NeuronIndex, Vector<double>> _neuronToSymbolicUbPerBranch;
    Map<NeuronIndex, Vector<double>> _neuronToSymbolicLowerBiasPerBranch;
    Map<NeuronIndex, Vector<double>> _neuronToSymbolicUpperBiasPerBranch;

    void freeMemoryIfNeeded();

    // Map each neuron to a linear expression representing its weighted sum
    void generateLinearExpressionForWeightedSumLayer(
        Map<unsigned, LinearExpression> &variableToExpression,
        const Layer &layer );

    // Helper functions for generating an input query
    void generateQueryForLayer( Query &inputQuery, const Layer &layer );
    void generateQueryForWeightedSumLayer( Query &inputQuery, const Layer &layer );
    void generateEquationsForWeightedSumLayer( List<Equation> &equations, const Layer &layer );
    void generateQueryForReluLayer( Query &inputQuery, const Layer &layer );
    void generateQueryForSigmoidLayer( Query &inputQuery, const Layer &layer );
    void generateQueryForSignLayer( Query &inputQuery, const Layer &layer );
    void generateQueryForAbsoluteValueLayer( Query &inputQuery, const Layer &layer );
    void generateQueryForMaxLayer( Query &inputQuery, const Layer &layer );

    bool suitableForMerging( unsigned secondLayerIndex,
                             const Map<unsigned, double> &lowerBounds,
                             const Map<unsigned, double> &upperBounds,
                             const Set<unsigned> &varsInConstraintsUnhandledByNLR );
    void mergeWSLayers( unsigned secondLayerIndex,
                        Map<unsigned, LinearExpression> &eliminatedNeurons );
    double *multiplyWeights( const double *firstMatrix,
                             const double *secondMatrix,
                             unsigned inputDimension,
                             unsigned middleDimension,
                             unsigned outputDimension );
    void reduceLayerIndex( unsigned layer, unsigned startIndex );

    // Return optimizable parameters which minimize parameterised SBT bounds' volume.
    const Vector<double> OptimalParameterisedSymbolicBoundTightening();

    // Optimize biases of generated parameterised polygonal tightenings.
    const Vector<PolygonalTightening> OptimizeParameterisedPolygonalTightening();

    // Estimate Volume of parameterised symbolic bound tightening.
    double EstimateVolume( const Vector<double> &coeffs );

    // Return difference between given point and upper and lower bounds determined by parameterised
    // SBT relaxation.
    double calculateDifferenceFromSymbolic( const Layer *layer,
                                            Map<unsigned, double> &point,
                                            unsigned i ) const;

    // Heuristically generating optimizable polygonal tightening for PMNR.
    const Vector<PolygonalTightening> generatePolygonalTighteningsForPMNR();

    // Heuristically select neurons for PMNR.
    const Vector<NeuronIndex> selectPMNRNeurons();

    // Optimize biases of generated parameterised polygonal tightenings.
    double OptimizeSingleParameterisedPolygonalTightening(
        PolygonalTightening &tightening,
        Vector<PolygonalTightening> &prevTightenings,
        bool maximize,
        double feasibilityBound,
        const Map<NeuronIndex, unsigned> &neuronToBranchIndex = Map<NeuronIndex, unsigned>( {} ) );

    double OptimizeSingleParameterisedPolygonalTighteningWithBranching(
        PolygonalTightening &tightening,
        Vector<PolygonalTightening> &prevTightenings,
        bool maximize,
        double bound );

    // Get current lower bound for selected parameterised polygonal tightenings' biases.
    double getParameterisdPolygonalTighteningBound(
        const Vector<double> &gamma,
        PolygonalTightening &tightening,
        Vector<PolygonalTightening> &prevTightenings,
        const Map<NeuronIndex, unsigned> &neuronToBranchIndex = Map<NeuronIndex, unsigned>( {} ) );

    /*
      Store previous biases for each ReLU neuron in a map for getPreviousBias()
      and BaBSR heuristic
    */
    Map<const ReluConstraint *, double> _previousBiases;
    void initializePreviousBiasMap();

    // Calculate PMNRScore for every non-fixed neurons.
    void initializePMNRScoreMap();
    double calculatePMNRBBPSScore( NeuronIndex index );

    // Initialize PMNR-BBPS branching point scores and per-branch predecessor symbolic bounds for
    // every non-fixed neuron.
    void initializeBBPSBranchingMaps();
    const std::pair<NeuronIndex, double> calculateBranchingPoint( NeuronIndex index ) const;

    // Heuristically generate candidates for branching points.
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidates( const Layer *layer, unsigned i ) const;

    // Helper functions for generating branching points.
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesAtZero( const Layer *layer, unsigned i ) const;
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesForRound( const Layer *layer, unsigned i ) const;
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesForSigmoid( const Layer *layer, unsigned i ) const;
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesForMax( const Layer *layer, unsigned i ) const;
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesForSoftmax( const Layer *layer, unsigned i ) const;
    const Vector<std::pair<NeuronIndex, double>>
    generateBranchingPointCandidatesForBilinear( const Layer *layer, unsigned i ) const;

    // Given neuron index, source index and branch ranges, compute symbolic bounds per branch.
    // If activation has multiple sources, sources other than given neuron are concretized.
    void calculateSymbolicBoundsPerBranch( NeuronIndex index,
                                           NeuronIndex sourceIndex,
                                           const Vector<double> &values,
                                           Vector<double> &symbolicLbPerBranch,
                                           Vector<double> &symbolicUbPerBranch,
                                           Vector<double> &symbolicLowerBiasPerBranch,
                                           Vector<double> &symbolicUpperBiasPerBranch,
                                           unsigned branchCount ) const;

    // Helper functions for calculating branch symbolic bounds.
    void
    calculateSymbolicBoundsPerBranchForRelu( unsigned i,
                                             double sourceLb,
                                             double sourceUb,
                                             Vector<double> &symbolicLbPerBranch,
                                             Vector<double> &symbolicUbPerBranch,
                                             Vector<double> &symbolicLowerBiasPerBranch,
                                             Vector<double> &symbolicUpperBiasPerBranch ) const;
    void calculateSymbolicBoundsPerBranchForAbsoluteValue(
        unsigned i,
        double sourceLb,
        double sourceUb,
        Vector<double> &symbolicLbPerBranch,
        Vector<double> &symbolicUbPerBranch,
        Vector<double> &symbolicLowerBiasPerBranch,
        Vector<double> &symbolicUpperBiasPerBranch ) const;
    void
    calculateSymbolicBoundsPerBranchForSign( unsigned i,
                                             double sourceLb,
                                             double sourceUb,
                                             Vector<double> &symbolicLbPerBranch,
                                             Vector<double> &symbolicUbPerBranch,
                                             Vector<double> &symbolicLowerBiasPerBranch,
                                             Vector<double> &symbolicUpperBiasPerBranch ) const;
    void
    calculateSymbolicBoundsPerBranchForRound( unsigned i,
                                              double sourceLb,
                                              double sourceUb,
                                              Vector<double> &symbolicLbPerBranch,
                                              Vector<double> &symbolicUbPerBranch,
                                              Vector<double> &symbolicLowerBiasPerBranch,
                                              Vector<double> &symbolicUpperBiasPerBranch ) const;
    void
    calculateSymbolicBoundsPerBranchForSigmoid( unsigned i,
                                                double sourceLb,
                                                double sourceUb,
                                                Vector<double> &symbolicLbPerBranch,
                                                Vector<double> &symbolicUbPerBranch,
                                                Vector<double> &symbolicLowerBiasPerBranch,
                                                Vector<double> &symbolicUpperBiasPerBranch ) const;
    void calculateSymbolicBoundsPerBranchForLeakyRelu(
        NeuronIndex index,
        unsigned i,
        double sourceLb,
        double sourceUb,
        Vector<double> &symbolicLbPerBranch,
        Vector<double> &symbolicUbPerBranch,
        Vector<double> &symbolicLowerBiasPerBranch,
        Vector<double> &symbolicUpperBiasPerBranch ) const;
    void calculateSymbolicBoundsPerBranchForMax( NeuronIndex index,
                                                 NeuronIndex chosenSourceIndex,
                                                 unsigned i,
                                                 double sourceLb,
                                                 double sourceUb,
                                                 Vector<double> &symbolicLbPerBranch,
                                                 Vector<double> &symbolicUbPerBranch,
                                                 Vector<double> &symbolicLowerBiasPerBranch,
                                                 Vector<double> &symbolicUpperBiasPerBranch ) const;
    void
    calculateSymbolicBoundsPerBranchForSoftmax( NeuronIndex index,
                                                NeuronIndex chosenSourceIndex,
                                                unsigned i,
                                                double sourceLb,
                                                double sourceUb,
                                                Vector<double> &symbolicLbPerBranch,
                                                Vector<double> &symbolicUbPerBranch,
                                                Vector<double> &symbolicLowerBiasPerBranch,
                                                Vector<double> &symbolicUpperBiasPerBranch ) const;
    void
    calculateSymbolicBoundsPerBranchForBilinear( NeuronIndex index,
                                                 NeuronIndex chosenSourceIndex,
                                                 unsigned i,
                                                 double sourceLb,
                                                 double sourceUb,
                                                 Vector<double> &symbolicLbPerBranch,
                                                 Vector<double> &symbolicUbPerBranch,
                                                 Vector<double> &symbolicLowerBiasPerBranch,
                                                 Vector<double> &symbolicUpperBiasPerBranch ) const;

    // Calculate tightening loss of branch symbolic bounds.
    double calculateTighteningLoss( const Vector<double> &values,
                                    const Vector<double> &symbolicLbPerBranch,
                                    const Vector<double> &symbolicUbPerBranch,
                                    const Vector<double> &symbolicLowerBiasPerBranch,
                                    const Vector<double> &symbolicUpperBiasPerBranch,
                                    unsigned branchCount ) const;

    // Get map containing vector of optimizable parameters for parameterised SBT relaxation for
    // every layer index.
    const Map<unsigned, Vector<double>>
    getParametersForLayers( const Vector<double> &coeffs ) const;

    // Get number of optimizable parameters for parameterised SBT relaxation per layer type.
    unsigned getNumberOfParametersPerType( Layer::Type t ) const;

    // Determine whether activation type and PMNR strategy support branching before INVPROP.
    bool supportsInvpropBranching( Layer::Type type ) const;

    // Get all indices of layers with non-fixed neurona.
    const Vector<unsigned> getLayersWithNonfixedNeurons() const;

    /*
      If the NLR is manipulated manually in order to generate a new
      input query, this method can be used to assign variable indices
      to all neurons in the network
    */
    void reindexNeurons();
};

} // namespace NLR

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
