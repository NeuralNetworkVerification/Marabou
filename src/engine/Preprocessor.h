/*********************                                                        */
/*! \file Preprocessor.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __Preprocessor_h__
#define __Preprocessor_h__

#include "Equation.h"
#include "InputQuery.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "Set.h"

class Preprocessor
{
public:
    Preprocessor();

    /*
      Main method of this class: preprocess the input query
    */
    InputQuery preprocess( const InputQuery &query, bool attemptVariableElimination = true );

    /*
      Have the preprocessor start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

    /*
      Obtain the values of variabels that have become fixed.
    */
    bool variableIsFixed( unsigned index ) const;
    double getFixedValue( unsigned index ) const;

    /*
      Obtain the values of variables that have been merged.
    */
    bool variableIsMerged( unsigned index ) const;
    unsigned getMergedIndex( unsigned index ) const;

    /*
      Obtain the new index of a variable.
    */
    unsigned getNewIndex( unsigned oldIndex ) const;

private:
    /*
      Transform all equations of type GE or LE to type EQ.
    */
    void makeAllEquationsEqualities();

	/*
      Tighten bounds using the linear equations
	*/
	bool processEquations();

    /*
      Tighten the bounds using the piecewise linear constraints
	*/
	bool processConstraints();

    /*
      If there exists an equation x = x', replace all instances of x with x'
    */
    bool processIdenticalVariables();

    /*
      Collect all variables whose lower and upper bounds are equal, or
      which do not appear anywhere in the input query.
    */
    void collectFixedValues();

    /*
      Separate the sets of merged and fixed variables. If x1 is merged into
      x2 and x2 is fixed, just mark x1 as fixed (instead of as merged).
    */
    void separateMergedAndFixed();

    /*
      Eliminate any variables that have become fixed or merged with an
      identical variable
	*/
	void eliminateVariables();

    /*
      Call on the PL constraints to add any auxiliary equations
    */
    void addPlAuxiliaryEquations();

    /*
      Attempt to figure out the network topology and construct a
      network level reasoner. Return true iff the construction was
      successful
    */
    bool constructNetworkLevelReasoner();

    /*
      The preprocessed query
    */
    InputQuery _preprocessed;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Variables that have become fixed during preprocessing, and the
      values that they have been fixed to.
    */
    Map<unsigned, double> _fixedVariables;

    /*
      Variables that have been merged with other varaibles, due to
      equations of the form x1 = x2
    */
    Map<unsigned, unsigned> _mergedVariables;

    /*
      Mapping of old variable indices to new varibale indices, if
      indices were changed during preprocessing.
    */
    Map<unsigned, unsigned> _oldIndexToNewIndex;

    /*
      For debugging only
    */
    void dumpAllBounds( const String &message );

    static void log( const String &message );

};

#endif // __Preprocessor_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
