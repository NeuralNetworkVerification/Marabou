/*********************                                                        */
/*! \file Preprocessor.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Preprocessor_h__
#define __Preprocessor_h__

#include "Equation.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "InputQuery.h"

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
      Eliminate any variables that have become files
	*/
	void eliminateFixedVariables();

    /*
      Call on the PL constraints to add any auxiliary equations
    */
    void addPlAuxiliaryEquations();

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
      Mapping of old variable indices to new varibale indices, if
      indices were changed during preprocessing.
    */
    Map<unsigned, unsigned> _oldIndexToNewIndex;
};

#endif // __Preprocessor_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
