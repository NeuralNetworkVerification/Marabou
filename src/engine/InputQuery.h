/*********************                                                        */
/*! \file InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __InputQuery_h__
#define __InputQuery_h__

#include "Equation.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "SymbolicBoundTightener.h"

class InputQuery
{
public:
    InputQuery();
    ~InputQuery();

    /*
      Methods for setting and getting the input part of the query
    */
    void setNumberOfVariables( unsigned numberOfVariables );
    void setLowerBound( unsigned variable, double bound );
    void setUpperBound( unsigned variable, double bound );

    void addEquation( const Equation &equation );

    unsigned getNumberOfVariables() const;
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;
    const Map<unsigned, double> &getLowerBounds() const;
    const Map<unsigned, double> &getUpperBounds() const;

    const List<Equation> &getEquations() const;
	List<Equation> &getEquations();

    void addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint );
    const List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints() const;
	List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints();

    /*
      Methods for handling input and output variables
    */
    void markInputVariable( unsigned variable, unsigned inputIndex );
    void markOutputVariable( unsigned variable, unsigned inputIndex );
    unsigned inputVariableByIndex( unsigned index ) const;
    unsigned outputVariableByIndex( unsigned index ) const;
    unsigned getNumInputVariables() const;
    unsigned getNumOutputVariables() const;
    List<unsigned> getInputVariables() const;

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
    void removeEquation(Equation e);

    /*
      Assignment operator and copy constructor, duplicate the constraints.
    */
    InputQuery &operator=( const InputQuery &other );
    InputQuery( const InputQuery &other );

    /*
      Attach a symbolic bound tightener to the input query
    */
    void setSymbolicBoundTightener( SymbolicBoundTightener *sbt );

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

    /*
      Print input and output bounds
    */
    void printInputOutputBounds() const;

    /*
      Adjsut the input/output variable mappings because variables have been merged
      or have become fixed
    */
    void adjustInputOutputMapping( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                   const Map<unsigned, unsigned> &mergedVariables );

private:
    unsigned _numberOfVariables;
    List<Equation> _equations;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    List<PiecewiseLinearConstraint *> _plConstraints;

    Map<unsigned, double> _solution;

    /*
      Free any stored pl constraints.
    */
    void freeConstraintsIfNeeded();

public:
    /*
      Mapping of input/output variables to their indices.
      Made public for easy access from the preprocessor.
    */
    Map<unsigned, unsigned> _variableToInputIndex;
    Map<unsigned, unsigned> _inputIndexToVariable;
    Map<unsigned, unsigned> _variableToOutputIndex;
    Map<unsigned, unsigned> _outputIndexToVariable;

    SymbolicBoundTightener *_sbt;
};

#endif // __InputQuery_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
