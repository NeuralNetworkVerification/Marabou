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
    /*
      Main method of this class: preprocess the input query
    */
    InputQuery preprocess( const InputQuery &query );

private:
	/*
      Tighten bounds using the linear equations
	*/
	bool processEquations();

    /*
      Tighten the bounds using the piecewise linear constraints
	*/
	bool processConstraints();

    /*
      Eliminate any variables that have become files
	*/
	void eliminateVariables();

    InputQuery _preprocessed;
};

#endif // __Preprocessor_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
