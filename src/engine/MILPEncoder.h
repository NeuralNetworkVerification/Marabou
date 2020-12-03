/*********************                                                        */
/*! \file MILPEncoder.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __MILPEncoder_h__
#define __MILPEncoder_h__

#include "GurobiWrapper.h"
#include "InputQuery.h"
#include "ITableau.h"
#include "MStringf.h"

#include "Map.h"

class MILPEncoder
{
public:
    MILPEncoder( const ITableau &tableau );

    /*
      Encode the input query as a Gurobi query, variables and inequalities
      are from inputQuery, and latest variable bounds are from tableau
    */
    void encodeInputQuery( GurobiWrapper &gurobi, const InputQuery &inputQuery );

    /*
      get variable name from a variable in the encoded inputquery
    */
    String getVariableNameFromVariable( unsigned variable );

private:

    /*
      Tableau has the latest bound
    */
    const ITableau &_tableau;

    /*
      Map the variable to the string encoded in Gurobi
    */
    Map<unsigned, String> _variableToVariableName;

    /*
      Index for Guroby binary variables
    */
    unsigned _binVarIndex = 0;

    /*
      Encode an (in)equality into Gurobi.
    */
    void encodeEquation( GurobiWrapper &gurobi, const Equation &Equation );

    /*
      Encode a ReLU constraint f = ReLU(b) into Gurobi using the same encoding in
      https://arxiv.org/pdf/1711.07356.pdf

      Encode the following constraint:

      a \in {0, 1}
      f <= b − lb_b(1 − a)
      f <= ub_b · a

      The other two constraints f >= b and f >= 0 are encoded already when
      preprocessing
    */
    void encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint *relu );

    /*
      Encode a MAX constraint y = max(x_1, x_2, ... ,x_m) into Gurobi using the same encoding in
      https://arxiv.org/pdf/1711.07356.pdf

      Encode the following constraint:

      y <= x_i + (1 - a_i) * (umax - l) & y >= x_i (i = 1 ~ m)
      a_1 + a_2 + ... + a_m = 1
      a_i \in {0, 1} (i = 1 ~ m)
    */
    void encodeMaxConstraint( GurobiWrapper &gurobi, MaxConstraint *max );
};

#endif // __MILPEncoder_h__
