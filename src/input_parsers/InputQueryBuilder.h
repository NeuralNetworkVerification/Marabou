/*********************                                                        */
/*! \file InputQueryBuilder.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This file provides a general interface for parsing a neural network file.
 ** Keeps track of internal state such as equations and variables that
 ** may be altered during parsing of a network. Once the network has been parsed
 ** they are then loaded into an Query.
 ** Future parsers for individual network formats should extend this interface.
 **/

#ifndef __InputQueryBuilder_h__
#define __InputQueryBuilder_h__

#include "DisjunctionConstraint.h"
#include "Equation.h"
#include "IQuery.h"
#include "LeakyReluConstraint.h"
#include "List.h"
#include "Map.h"
#include "MaxConstraint.h"
#include "NonlinearConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "ReluConstraint.h"
#include "SigmoidConstraint.h"
#include "SignConstraint.h"
#include "Vector.h"

#include <utility>

typedef unsigned int Variable;

class InputQueryBuilder
{
private:
    unsigned int _numVars;
    List<Variable> _inputVars;
    List<Variable> _outputVars;

    Vector<Equation> _equationList;
    List<ReluConstraint *> _reluList;
    List<LeakyReluConstraint *> _leakyReluList;
    List<SigmoidConstraint *> _sigmoidList;
    List<MaxConstraint *> _maxList;
    List<AbsoluteValueConstraint *> _absList;
    List<SignConstraint *> _signList;
    Map<Variable, float> _lowerBounds;
    Map<Variable, float> _upperBounds;

public:
    InputQueryBuilder();

    Variable getNewVariable();

    void markInputVariable( Variable var );
    void markOutputVariable( Variable var );
    void addEquation( Equation &eq );
    void setLowerBound( Variable var, float value );
    void setUpperBound( Variable var, float value );
    void addRelu( Variable var1, Variable var2 );
    void addLeakyRelu( Variable var1, Variable var2, float alpha );
    void addSigmoid( Variable var1, Variable var2 );
    void addTanh( Variable var1, Variable var2 );
    void addSignConstraint( Variable var1, Variable var2 );
    void addMaxConstraint( Variable maxVar, Set<Variable> elements );
    void addAbsConstraint( Variable var1, Variable var2 );

    void generateQuery( IQuery &query );

    Equation *findEquationWithOutputVariable( Variable variable );
    virtual ~InputQueryBuilder();
};

#endif // __InputQueryBuilder_h__
