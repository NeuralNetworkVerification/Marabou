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
#include "Map.h"
#include "PiecewiseLinearFunctionType.h"
#include "Tightening.h"

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner
{
public:
    struct Index
    {
        Index()
            : _layer( 0 )
            , _neuron( 0 )
        {
        }

        Index( unsigned layer, unsigned neuron )
            : _layer( layer )
            , _neuron( neuron )
        {
        }

        bool operator<( const Index &other ) const
        {
            if ( _layer < other._layer )
                return true;
            if ( _layer > other._layer )
                return false;

            return _neuron < other._neuron;
        }

        unsigned _layer;
        unsigned _neuron;
    };

    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    static bool functionTypeSupported( PiecewiseLinearFunctionType type );

    void setNumberOfLayers( unsigned numberOfLayers );
    void setLayerSize( unsigned layer, unsigned size );
    void setNeuronActivationFunction( unsigned layer, unsigned neuron, PiecewiseLinearFunctionType activationFuction );
    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight );
    void setBias( unsigned layer, unsigned neuron, double bias );

    /*
      A method that allocates all internal memory structures, based on
      the network's topology. Should be invoked after the layer sizes
      have been provided.
    */
    void allocateMemoryByTopology();

    /*
      Mapping from node indices to the variables representing their
      weighted sum values and activation result values.
    */
    void setWeightedSumVariable( unsigned layer, unsigned neuron, unsigned variable );
    unsigned getWeightedSumVariable( unsigned layer, unsigned neuron ) const;
    void setActivationResultVariable( unsigned layer, unsigned neuron, unsigned variable );
    unsigned getActivationResultVariable( unsigned layer, unsigned neuron ) const;
    const Map<Index, unsigned> &getIndexToWeightedSumVariable();
    const Map<Index, unsigned> &getIndexToActivationResultVariable();

    /*
      Mapping from node indices to the nodes' assignments, as computed
      by evaluate()
    */
    const Map<Index, double> &getIndexToWeightedSumAssignment();
    const Map<Index, double> &getIndexToActivationResultAssignment();

    /*
      Interface methods for performing operations on the network.
    */
    void evaluate( double *input, double *output );

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
    void obtainCurrentBounds();
    void intervalArithmeticBoundPropagation();
    void symbolicBoundPropagation();

    void getConstraintTightenings( List<Tightening> &tightenings ) const;

    /*
      For debugging purposes: dump the network topology
    */
    void dumpTopology() const;

private:
    unsigned _numberOfLayers;
    Map<unsigned, unsigned> _layerSizes;
    Map<Index, PiecewiseLinearFunctionType> _neuronToActivationFunction;
    double **_weights;
    double **_positiveWeights;
    double **_negativeWeights;
    Map<Index, double> _bias;

    unsigned _maxLayerSize;
    unsigned _inputLayerSize;

    double *_work1;
    double *_work2;

    const ITableau *_tableau;

    void freeMemoryIfNeeded();

    /*
      Mappings of indices to weighted sum and activation result variables
    */
    Map<Index, unsigned> _indexToWeightedSumVariable;
    Map<Index, unsigned> _indexToActivationResultVariable;
    Map<unsigned, Index> _weightedSumVariableToIndex;
    Map<unsigned, Index> _activationResultVariableToIndex;

    /*
      Store the assignment to all variables when evaluate() is called
    */
    Map<Index, double> _indexToWeightedSumAssignment;
    Map<Index, double> _indexToActivationResultAssignment;

    /*
      Store eliminated variables
    */
    Map<Index, double> _eliminatedWeightedSumVariables;
    Map<Index, double> _eliminatedActivationResultVariables;

    /*
      Work space for bound tightening
    */
    double **_lowerBoundsWeightedSums;
    double **_upperBoundsWeightedSums;
    double **_lowerBoundsActivations;
    double **_upperBoundsActivations;

    /*
      Work space for symbolic bound propagation
    */
    double *_currentLayerLowerBounds;
    double *_currentLayerUpperBounds;
    double *_currentLayerLowerBias;
    double *_currentLayerUpperBias;

    double *_previousLayerLowerBounds;
    double *_previousLayerUpperBounds;
    double *_previousLayerLowerBias;
    double *_previousLayerUpperBias;

    /*
      Helper functions that perform symbolic bound propagation for a
      single neuron, according to its activation function
    */
    void reluSymbolicPropagation( const Index &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );
    void absoluteValueSymbolicPropagation( const Index &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );

    static void log( const String &message );
};

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
