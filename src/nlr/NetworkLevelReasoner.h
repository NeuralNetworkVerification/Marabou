/*********************                                                        */
/*! \file NetworkLevelReasoner.h
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

#ifndef __NetworkLevelReasoner_h__
#define __NetworkLevelReasoner_h__

#include "ITableau.h"
#include "Layer.h"
#include "LayerOwner.h"
#include "Map.h"
#include "MatrixMultiplication.h"
#include "NeuronIndex.h"
#include "PiecewiseLinearFunctionType.h"
#include "Tightening.h"

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
    void evaluate( double *input , double *output );

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

        - LP Relaxation: invoking an LP solver on a series of LP
          relaxations of the problem we're trying to solve, and
          optimizing the lower and upper bounds of each of the
          varibales.

        - receiveTighterBound: this is a callback from the layer
          objects, through which they report tighter bounds.

        - getConstraintTightenings: this is the function that an
          external user calls in order to collect the tighter bounds
          discovered by the NLR.
    */

    void setTableau( const ITableau *tableau );
    const ITableau *getTableau() const;

    void obtainCurrentBounds();
    void intervalArithmeticBoundPropagation();
    void symbolicBoundPropagation();
    void lpRelaxationPropagation();
    void MILPPropagation();
    void iterativePropagation();

    void receiveTighterBound( Tightening tightening );
    void getConstraintTightenings( List<Tightening> &tightenings );

    /*
      For debugging purposes: dump the network topology
    */
    void dumpTopology() const;

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
      Generate an input query from this NLR, according to the
      discovered network topology
    */
    InputQuery generateInputQuery();

    /*
      Finds logically consecutive WS layers and merges them, in order
      to reduce the total number of layers and variables in the
      network
    */
    void mergeConsecutiveWSLayers();

    /*
      Print the bounds of variables layer by layer
    */
    void dumpBounds();

    /*
      Get the size of the widest layer
    */
    unsigned getMaxLayerSize() const;

private:
    Map<unsigned, Layer *> _layerIndexToLayer;
    const ITableau *_tableau;

    // Tightenings discovered by the various layers
    List<Tightening> _boundTightenings;

    void freeMemoryIfNeeded();

    List<PiecewiseLinearConstraint *> _constraintsInTopologicalOrder;

    // Helper functions for generating an input query
    void generateInputQueryForLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForWeightedSumLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForReluLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForSignLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForAbsoluteValueLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForMaxLayer( InputQuery &inputQuery, const Layer &layer );

    bool suitableForMerging( unsigned secondLayerIndex );
    void mergeWSLayers( unsigned secondLayerIndex );
    double *multiplyWeights( const double *firstMatrix,
                             const double *secondMatrix,
                             unsigned inputDimension,
                             unsigned middleDimension,
                             unsigned outputDimension );
    void reduceLayerIndex( unsigned layer, unsigned startIndex );

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
