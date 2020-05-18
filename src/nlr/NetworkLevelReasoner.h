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

#include "NeuronIndex.h"
#include "ITableau.h"
#include "Map.h"
#include "PiecewiseLinearFunctionType.h"
#include "Tightening.h"
#include "Layer.h"

namespace NLR {

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner : public Layer::LayerOwner
{
public:
    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    static bool functionTypeSupported( PiecewiseLinearFunctionType type );

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
    const Layer *getLayer( unsigned index ) const;

    /*
      Interface methods for performing operations on the network.
    */
    void evaluate( double *input , double *output );

    void setNeuronVariable( NeuronIndex index, unsigned variable );

    /*
      Bound propagation methods:

        - obtainCurrentBounds: obtain the current bounds on all variables
          from the tableau.

        - Interval arithmetic: compute the bounds of a layer's neurons
          based on the concrete bounds of the previous layer.

        - Symbolic: for each neuron in the network, we compute lower
          and upper bounds on the lower and upper bounds of the
          neuron. This bounds are expressed as linear combinations of
          the input neurons. Sometimes these bounds let us simplify
          expressions and obtain tighter bounds (e.g., if the upper
          bound on the upper bound of a ReLU node is negative, that
          ReLU is inactive and its output can be set to 0.

          Initialize should be called once, before the bound
          propagation is performed.
    */

    void setTableau( const ITableau *tableau );
    const ITableau *getTableau() const;

    void obtainCurrentBounds();
    // void intervalArithmeticBoundPropagation();
    // void symbolicBoundPropagation();

    // void getConstraintTightenings( List<Tightening> &tightenings ) const;

    // /*
    //   For debugging purposes: dump the network topology
    // */
    // void dumpTopology() const;





    /*
      Mapping from node indices to the variables representing their
      weighted sum values and activation result values.
    */
    // void setWeightedSumVariable( unsigned layer, unsigned neuron, unsigned variable );
    // unsigned getWeightedSumVariable( unsigned layer, unsigned neuron ) const;
    // void setActivationResultVariable( unsigned layer, unsigned neuron, unsigned variable );
    // unsigned getActivationResultVariable( unsigned layer, unsigned neuron ) const;
    // const Map<NeuronIndex, unsigned> &getIndexToWeightedSumVariable();
    // const Map<NeuronIndex, unsigned> &getIndexToActivationResultVariable();

    /*
      Mapping from node indices to the nodes' assignments, as computed
      by evaluate()
    */
    // const Map<NeuronIndex, double> &getIndexToWeightedSumAssignment();
    // const Map<NeuronIndex, double> &getIndexToActivationResultAssignment();

    /*
      Duplicate the reasoner
    */
    // void storeIntoOther( NetworkLevelReasoner &other ) const;

    /*
      Methods that are typically invoked by the preprocessor, to
      inform us of changes in variable indices or if a variable has
      been eliminated
    */
    // void eliminateVariable( unsigned variable, double value );
    // void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
    //                             const Map<unsigned, unsigned> &mergedVariables );

private:
    Map<unsigned, Layer *> _layerIndexToLayer;

// private:
//     unsigned _numberOfLayers;
//     Map<unsigned, unsigned> _layerSizes;
//     Map<NeuronIndex, PiecewiseLinearFunctionType> _neuronToActivationFunction;
//     double **_weights;
//     double **_positiveWeights;
//     double **_negativeWeights;
//     Map<NeuronIndex, double> _bias;

//     unsigned _maxLayerSize;
//     unsigned _inputLayerSize;

//     double *_work1;
//     double *_work2;

    const ITableau *_tableau;

//     void freeMemoryIfNeeded();

//     /*
//       Mappings of indices to weighted sum and activation result variables
//     */
//     Map<NeuronIndex, unsigned> _indexToWeightedSumVariable;
//     Map<NeuronIndex, unsigned> _indexToActivationResultVariable;
//     Map<unsigned, NeuronIndex> _weightedSumVariableToIndex;
//     Map<unsigned, NeuronIndex> _activationResultVariableToIndex;

//     /*
//       Store the assignment to all variables when evaluate() is called
//     */
//     Map<NeuronIndex, double> _indexToWeightedSumAssignment;
//     Map<NeuronIndex, double> _indexToActivationResultAssignment;

//     /*
//       Store eliminated variables
//     */
//     Map<NeuronIndex, double> _eliminatedWeightedSumVariables;
//     Map<NeuronIndex, double> _eliminatedActivationResultVariables;

//     /*
//       Work space for bound tightening
//     */
//     double **_lowerBoundsWeightedSums;
//     double **_upperBoundsWeightedSums;
//     double **_lowerBoundsActivations;
//     double **_upperBoundsActivations;

//     /*
//       Work space for symbolic bound propagation
//     */
//     double *_currentLayerLowerBounds;
//     double *_currentLayerUpperBounds;
//     double *_currentLayerLowerBias;
//     double *_currentLayerUpperBias;

//     double *_previousLayerLowerBounds;
//     double *_previousLayerUpperBounds;
//     double *_previousLayerLowerBias;
//     double *_previousLayerUpperBias;

//     /*
//       Helper functions that perform symbolic bound propagation for a
//       single neuron, according to its activation function
//     */
//     void reluSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );
//     void absoluteValueSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );

//     static void log( const String &message );
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
