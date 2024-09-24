/*********************                                                        */
/*! \file Preprocessor.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __Preprocessor_h__
#define __Preprocessor_h__

#include "Equation.h"
#include "LinearExpression.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "Query.h"
#include "Set.h"

class Preprocessor
{
public:
    Preprocessor();

    ~Preprocessor();

    /*
      Main method of this class: preprocess the input query
    */
    std::unique_ptr<Query> preprocess( const IQuery &query,
                                       bool attemptVariableElimination = true );

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
      Check whether a variable is unused by symbolically fixed.
    */
    bool variableIsUnusedAndSymbolicallyFixed( unsigned index ) const;

    /*
      Obtain the new index of a variable.
    */
    unsigned getNewIndex( unsigned oldIndex ) const;

    /*
      Given an inputQuery with all variable assignment other than ones for
      variables corresponding to eliminated neurons, compute the full
      assignment.
    */
    void setSolutionValuesOfEliminatedNeurons( IQuery &inputQuery );

    static void informConstraintsOfInitialBounds( Query &query );

private:
    void freeMemoryIfNeeded();

    inline double getLowerBound( unsigned var )
    {
        return _lowerBounds[var];
    }

    inline double getUpperBound( unsigned var )
    {
        return _upperBounds[var];
    }

    inline void setLowerBound( unsigned var, double value )
    {
        _lowerBounds[var] = value;
    }

    inline void setUpperBound( unsigned var, double value )
    {
        _upperBounds[var] = value;
    }

    /*
      Transform each equation so that any two addends have different variables
      and no addends have zero coefficients
    */
    void removeRedundantAddendsInAllEquations();

    /*
      Transform the piecewise linear constraints if needed. For instance, this
      will make sure all disjuncts in all disjunctions contain only bounds and
      no (in)equalities between variables.
    */
    void transformConstraintsIfNeeded();

    /*
      Transform all equations of type GE or LE to type EQ.
    */
    void makeAllEquationsEqualities();

    /*
      Set any missing upper bound to +INF, and any missing lower bound
      to -INF.
    */
    void setMissingBoundsToInfinity();

    /*
      Tighten bounds using the linear equations
    */
    bool processEquations();

    /*
      Tighten the bounds using the piecewise linear and nonlinear constraints
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
      All input/output variables
    */
    Set<unsigned> _uneliminableVariables;

    /*
      The preprocessed query
    */
    std::unique_ptr<Query> _preprocessed;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Used to store the bounds during the preprocessing.
    */
    double *_lowerBounds;
    double *_upperBounds;

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
      Variables that have been eliminated due to the merge of weighted
      sum layers. These variables are symbolically fixed (as a weighted
      sum of other variables), not needed for solving, but need to be
      tracked here for reconstruction of the full assignment.
    */
    Map<unsigned, LinearExpression> _unusedSymbolicallyFixedVariables;

    /*
      Mapping of old variable indices to new varibale indices, if
      indices were changed during preprocessing.
    */
    Map<unsigned, unsigned> _oldIndexToNewIndex;

    /*
      For debugging only
    */
    void dumpAllBounds( const String &message );
};

#endif // __Preprocessor_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
