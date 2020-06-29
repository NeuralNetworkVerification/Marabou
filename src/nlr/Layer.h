/*********************                                                        */
/*! \file Layer.h
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

#ifndef __Layer_h__
#define __Layer_h__

#include "AbsoluteValueConstraint.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "LayerOwner.h"
#include "MarabouError.h"
#include "MatrixMultiplication.h"
#include "NeuronIndex.h"
#include "ReluConstraint.h"

namespace NLR {

class Layer
{
public:
    enum Type {
        // Linear layers
        INPUT = 0,
        WEIGHTED_SUM,

        // Activation functions
        RELU,
        ABSOLUTE_VALUE,
        MAX,
    };

    /*
      Construct a layer directly and populate its fields, or clone
      from another layer
    */
    Layer( const Layer *other );
    Layer( unsigned index, Type type, unsigned size, LayerOwner *layerOwner );
    ~Layer();

    void setLayerOwner( LayerOwner *layerOwner );
    void addSourceLayer( unsigned layerNumber, unsigned layerSize );
    const Map<unsigned, unsigned> &getSourceLayers() const;

    void setWeight( unsigned sourceLayer,
                    unsigned sourceNeuron,
                    unsigned targetNeuron,
                    double weight );
    double getWeight( unsigned sourceLayer,
                      unsigned sourceNeuron,
                      unsigned targetNeuron ) const;

    void setBias( unsigned neuron, double bias );
    double getBias( unsigned neuron ) const;

    void addActivationSource( unsigned sourceLayer,
                              unsigned sourceNeuron,
                              unsigned targetNeuron );
    List<NeuronIndex> getActivationSources( unsigned neuron ) const;

    void setNeuronVariable( unsigned neuron, unsigned variable );
    bool neuronHasVariable( unsigned neuron ) const;
    unsigned neuronToVariable( unsigned neuron ) const;
    unsigned variableToNeuron( unsigned variable ) const;

    unsigned getSize() const;
    unsigned getLayerIndex() const;
    Type getLayerType() const;

    /*
      Set/get the assignment, or compute it from source layers
    */
    void setAssignment( const double *values );
    const double *getAssignment() const;
    double getAssignment( unsigned neuron ) const;
    void computeAssignment();

    /*
      Bound related functionality: grab the current bounds from the
      Tableau, or compute bounds from source layers
    */
    void setLb( unsigned neuron, double bound );
    void setUb( unsigned neuron, double bound );
    double getLb( unsigned neuron ) const;
    double getUb( unsigned neuron ) const;

    void obtainCurrentBounds();
    void computeSymbolicBounds();
    void computeIntervalArithmeticBounds();

    /*
      Preprocessing functionality: variable elimination and reindexing
    */
    void eliminateVariable( unsigned variable, double value );
    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables );

    bool neuronEliminated( unsigned neuron ) const;
    double getEliminatedNeuronValue( unsigned neuron ) const;

    /*
      For debugging purposes
    */
    void dump() const;
    static String typeToString( Type type );

private:
    unsigned _layerIndex;
    Type _type;
    unsigned _size;
    LayerOwner *_layerOwner;

    Map<unsigned, unsigned> _sourceLayers;

    Map<unsigned, double *> _layerToWeights;
    Map<unsigned, double *> _layerToPositiveWeights;
    Map<unsigned, double *> _layerToNegativeWeights;
    double *_bias;

    double *_assignment;

    double *_lb;
    double *_ub;

    Map<unsigned, List<NeuronIndex>> _neuronToActivationSources;

    Map<unsigned, unsigned> _neuronToVariable;
    Map<unsigned, unsigned> _variableToNeuron;
    Map<unsigned, double> _eliminatedNeurons;

    unsigned _inputLayerSize;
    double *_symbolicLb;
    double *_symbolicUb;
    double *_symbolicLowerBias;
    double *_symbolicUpperBias;
    double *_symbolicLbOfLb;
    double *_symbolicUbOfLb;
    double *_symbolicLbOfUb;
    double *_symbolicUbOfUb;

    void allocateMemory();
    void freeMemoryIfNeeded();

    /*
      Helper functions for symbolic bound tightening
    */
    void comptueSymbolicBoundsForInput();
    void computeSymbolicBoundsForRelu();
    void computeSymbolicBoundsForAbsoluteValue();
    void computeSymbolicBoundsForWeightedSum();

    /*
      Helper functions for interval bound tightening
    */
    void computeIntervalArithmeticBoundsForWeightedSum();
    void computeIntervalArithmeticBoundsForRelu();
    void computeIntervalArithmeticBoundsForAbs();

    const double *getSymbolicLb() const;
    const double *getSymbolicUb() const;
    const double *getSymbolicLowerBias() const;
    const double *getSymbolicUpperBias() const;
    double getSymbolicLbOfLb( unsigned neuron ) const;
    double getSymbolicUbOfLb( unsigned neuron ) const;
    double getSymbolicLbOfUb( unsigned neuron ) const;
    double getSymbolicUbOfUb( unsigned neuron ) const;
};

} // namespace NLR

#endif // __Layer_h__
