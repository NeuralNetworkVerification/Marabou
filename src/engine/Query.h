/*********************                                                        */
/*! \file Query.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor, Derek Huang, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 **	This class represents a non-incremental verification query and is intended
 ** for internal use only (i.e., should not be exposed to the users).
 **
 **/

#ifndef __Query_h__
#define __Query_h__

#include "Equation.h"
#include "IQuery.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "NetworkLevelReasoner.h"
#include "NonlinearConstraint.h"
#include "PiecewiseLinearConstraint.h"

class InputQuery;

class Query : public IQuery
{
public:
    Query();
    ~Query();

    /*
      Methods for setting and getting the input part of the query
    */
    void setNumberOfVariables( unsigned numberOfVariables );
    void setLowerBound( unsigned variable, double bound );
    void setUpperBound( unsigned variable, double bound );
    bool tightenLowerBound( unsigned variable, double bound );
    bool tightenUpperBound( unsigned variable, double bound );

    void addEquation( const Equation &equation );
    unsigned getNumberOfEquations() const;

    unsigned getNumberOfVariables() const;
    unsigned getNewVariable();
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;
    const Map<unsigned, double> &getLowerBounds() const;
    const Map<unsigned, double> &getUpperBounds() const;
    void clearBounds();

    const List<Equation> &getEquations() const;
    List<Equation> &getEquations();
    void removeEquationsByIndex( const Set<unsigned> indices );

    void addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint );
    const List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints() const;
    List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints();

    // Encode a clip constraint using two ReLU constraints
    void addClipConstraint( unsigned b, unsigned f, double floor, double ceiling );

    void addNonlinearConstraint( NonlinearConstraint *constraint );
    void getNonlinearConstraints( Vector<NonlinearConstraint *> &constraints ) const;

    const List<NonlinearConstraint *> &getNonlinearConstraints() const;
    List<NonlinearConstraint *> &getNonlinearConstraints();

    /*
      Methods for handling input and output variables
    */
    void markInputVariable( unsigned variable, unsigned inputIndex );
    void markOutputVariable( unsigned variable, unsigned outputIndex );
    unsigned inputVariableByIndex( unsigned index ) const;
    unsigned outputVariableByIndex( unsigned index ) const;
    unsigned getNumInputVariables() const;
    unsigned getNumOutputVariables() const;
    List<unsigned> getInputVariables() const;
    List<unsigned> getOutputVariables() const;

    /*
      Methods for setting and getting the solution.
    */
    void setSolutionValue( unsigned variable, double value );
    double getSolutionValue( unsigned variable ) const;

    /*
      Count the number of infinite bounds in the input query.
    */
    unsigned countInfiniteBounds();

    /*
      If two variables are known to be identical, merge them.
      (v1 will be merged into v2).
    */
    void mergeIdenticalVariables( unsigned v1, unsigned v2 );

    /*
      Remove an equation from equation list
    */
    void removeEquation( Equation e );

    /*
      Assignment operator and copy constructor, duplicate the constraints.
    */
    Query &operator=( const Query &other );
    Query( const Query &other );

    /*
      Debugging methods
    */

    /*
      Store a correct possible solution
    */
    void storeDebuggingSolution( unsigned variable, double value );
    Map<unsigned, double> _debuggingSolution;

    /*
      Serializes the query to a file which can then be loaded using QueryLoader.
    */
    void saveQuery( const String &fileName );
    void saveQueryAsSmtLib( const String &fileName ) const;

    /*
      Print input and output bounds
    */
    void printInputOutputBounds() const;
    void dump() const;
    void printAllBounds() const;

    /*
      Adjsut the input/output variable mappings because variables have been merged
      or have become fixed
    */
    void adjustInputOutputMapping( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                   const Map<unsigned, unsigned> &mergedVariables );

    /*
      Attempt to figure out the network topology and construct a
      network level reasoner. Also collect the equations that are not handled
      by the NLR, as well as variables that participate in
      equations/plconstraints/nlconstraints that are not handled by the NLR.

      Return true iff the construction was successful
    */
    bool constructNetworkLevelReasoner( List<Equation> &unhandledEquations,
                                        Set<unsigned> &varsInUnhandledConstraints );

    /*
      Merge consecutive weighted sum layers, and update _equation accordingly.
      Equalities corresponding to the merge layers are added, and equalities
      corresponding to pre-merged layers are removed.
    */
    void mergeConsecutiveWeightedSumLayers( const List<Equation> &unhandledEquations,
                                            const Set<unsigned> &varsInUnhandledConstraints,
                                            Map<unsigned, LinearExpression> &eliminatedNeurons );

    /*
      Include a network level reasoner in the query
    */
    void setNetworkLevelReasoner( NLR::NetworkLevelReasoner *nlr );
    NLR::NetworkLevelReasoner *getNetworkLevelReasoner() const;

    // A map for storing the tableau aux variable assigned to each PLC
    Map<unsigned, unsigned> _lastAddendToAux;

    Query *generateQuery() const;

private:
    unsigned _numberOfVariables;
    List<Equation> _equations;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    List<PiecewiseLinearConstraint *> _plConstraints;
    List<NonlinearConstraint *> _nlConstraints;

    Map<unsigned, double> _solution;

    /*
      Attempt to ensure that neurons in the same non-linear NLR layer
      have the same source layer by spacing neurons with different
      source neurons in separate NLR layers.
    */
    bool _ensureSameSourceLayerInNLR;

    /*
      Free any stored pl constraints.
    */
    void freeConstraintsIfNeeded();

    /*
      Methods called by constructNetworkLevelReasoner
    */
    bool constructWeighedSumLayer( NLR::NetworkLevelReasoner *nlr,
                                   Map<unsigned, unsigned> &handledVariableToLayer,
                                   unsigned newLayerIndex,
                                   Set<unsigned> &handledEquations );
    bool constructRoundLayer( NLR::NetworkLevelReasoner *nlr,
                              Map<unsigned, unsigned> &handledVariableToLayer,
                              unsigned newLayerIndex,
                              Set<NonlinearConstraint *> &handledNLConstraints );
    bool constructReluLayer( NLR::NetworkLevelReasoner *nlr,
                             Map<unsigned, unsigned> &handledVariableToLayer,
                             unsigned newLayerIndex,
                             Set<PiecewiseLinearConstraint *> &handledPLConstraints );
    bool constructLeakyReluLayer( NLR::NetworkLevelReasoner *nlr,
                                  Map<unsigned, unsigned> &handledVariableToLayer,
                                  unsigned newLayerIndex,
                                  Set<PiecewiseLinearConstraint *> &handledPLConstraints );
    bool constructSigmoidLayer( NLR::NetworkLevelReasoner *nlr,
                                Map<unsigned, unsigned> &handledVariableToLayer,
                                unsigned newLayerIndex,
                                Set<NonlinearConstraint *> &handledNLConstraints );
    bool constructAbsoluteValueLayer( NLR::NetworkLevelReasoner *nlr,
                                      Map<unsigned, unsigned> &handledVariableToLayer,
                                      unsigned newLayerIndex,
                                      Set<PiecewiseLinearConstraint *> &handledPLConstraints );
    bool constructSignLayer( NLR::NetworkLevelReasoner *nlr,
                             Map<unsigned, unsigned> &handledVariableToLayer,
                             unsigned newLayerIndex,
                             Set<PiecewiseLinearConstraint *> &handledPLConstraints );
    bool constructMaxLayer( NLR::NetworkLevelReasoner *nlr,
                            Map<unsigned, unsigned> &handledVariableToLayer,
                            unsigned newLayerIndex,
                            Set<PiecewiseLinearConstraint *> &handledPLConstraints );
    bool constructBilinearLayer( NLR::NetworkLevelReasoner *nlr,
                                 Map<unsigned, unsigned> &handledVariableToLayer,
                                 unsigned newLayerIndex,
                                 Set<NonlinearConstraint *> &handledNLConstraints );
    bool constructSoftmaxLayer( NLR::NetworkLevelReasoner *nlr,
                                Map<unsigned, unsigned> &handledVariableToLayer,
                                unsigned newLayerIndex,
                                Set<NonlinearConstraint *> &handledNLConstraints );

public:
    /*
      Mapping of input/output variables to their indices.
      Made public for easy access from the preprocessor.
    */
    Map<unsigned, unsigned> _variableToInputIndex;
    Map<unsigned, unsigned> _inputIndexToVariable;
    Map<unsigned, unsigned> _variableToOutputIndex;
    Map<unsigned, unsigned> _outputIndexToVariable;

    /*
      An object that knows the topology of the network being checked,
      and can be used for various operations such as network
      evaluation of topology-based bound tightening.
     */
    NLR::NetworkLevelReasoner *_networkLevelReasoner;
};

#endif // __Query_h__
