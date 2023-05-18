/*********************                                                        */
/*! \file NetworkParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This file provides a general interface for parsing a neural network file.
 ** Keeps track of internal state such as equations and variables that
 ** may be altered during parsing of a network. Once the network has been parsed
 ** they are then loaded into an InputQuery.
 ** Future parsers for individual network formats should extend this interface.
 **/

#ifndef __NetworkParser_h__
#define __NetworkParser_h__

#include "Map.h"
#include "List.h"
#include "Vector.h"
#include "Equation.h"
#include "InputQuery.h"
#include "ReluConstraint.h"
#include "DisjunctionConstraint.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "SigmoidConstraint.h"
#include "SignConstraint.h"
#include "TranscendentalConstraint.h"
#include <utility>

typedef unsigned int Variable;

class NetworkParser {
private:
    unsigned int _numVars;

protected:
    List<Variable> _inputVars;
    List<Variable> _outputVars;

    Vector<Equation> _equationList;
    List<ReluConstraint*> _reluList;
    List<SigmoidConstraint*> _sigmoidList;
    List<MaxConstraint*> _maxList;
    List<AbsoluteValueConstraint*> _absList;
    List<SignConstraint*> _signList;
    Map<Variable,float> _lowerBounds;
    Map<Variable,float> _upperBounds;

    NetworkParser();
    void initNetwork();

    void addEquation( Equation &eq );
    void setLowerBound( Variable var, float value );
    void setUpperBound( Variable var, float value );
    void addRelu( Variable var1, Variable var2 );
    void addSigmoid( Variable var1, Variable var2 );
    void addSignConstraint( Variable var1, Variable var2 );
    void addMaxConstraint( Variable maxVar, Set<Variable> elements );
    void addAbsConstraint( Variable var1, Variable var2 );

    Variable getNewVariable();
    void getMarabouQuery( InputQuery& query );

    int findEquationWithOutputVariable( Variable variable );
};

#endif // __NetworkParser_h__