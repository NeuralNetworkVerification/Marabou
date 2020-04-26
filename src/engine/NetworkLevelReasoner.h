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

    /*
      Interface methods for populating the network: settings its
      number of layers and the layer sizes, kinds of activation
      functions, weights and biases, etc.
    */
    enum ActivationFunction {
        ReLU,
    };

    void setNumberOfLayers( unsigned numberOfLayers );
    void setLayerSize( unsigned layer, unsigned size );
    void setNeuronActivationFunction( unsigned layer, unsigned neuron, ActivationFunction activationFuction );
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
      Methods that are typically invoked by the preprocessor,
      to inform us of changes in variable indices
    */
    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables );

    /*
      Bound propagation methods:

        - obtainInputBounds: obtain the current bounds on input variables
          from the tableau.

        - Interval arithmetic: compute the bounds of a layer's neurons
          based on the concrete bounds of the previous layer.
    */

    void setTableau( const ITableau *tableau );
    void obtainInputBounds();
    void intervalArithmeticBoundPropagation();

    void getConstraintTightenings( List<Tightening> &tightenings ) const;

private:
    unsigned _numberOfLayers;
    Map<unsigned, unsigned> _layerSizes;
    Map<Index, ActivationFunction> _neuronToActivationFunction;
    double **_weights;
    Map<Index, double> _bias;

    unsigned _maxLayerSize;

    double *_work1;
    double *_work2;

    const ITableau *_tableau;

    void freeMemoryIfNeeded();

    /*
      Mappings of indices to weighted sum and activation result variables
    */
    Map<Index, unsigned> _indexToWeightedSumVariable;
    Map<Index, unsigned> _indexToActivationResultVariable;

    /*
      Store the assignment to all variables when evaluate() is called
    */
    Map<Index, double> _indexToWeightedSumAssignment;
    Map<Index, double> _indexToActivationResultAssignment;

    /*
      Work space for bound tightening.
    */
    double **_lowerBoundsWeightedSums;
    double **_upperBoundsWeightedSums;
    double **_lowerBoundsActivations;
    double **_upperBoundsActivations;
};

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
