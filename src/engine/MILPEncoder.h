/*********************                                                        */
/*! \file MILPEncoder.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __MILPEncoder_h__
#define __MILPEncoder_h__

#include "BilinearConstraint.h"
#include "DisjunctionConstraint.h"
#include "GurobiWrapper.h"
#include "ITableau.h"
#include "LeakyReluConstraint.h"
#include "LinearExpression.h"
#include "MStringf.h"
#include "Map.h"
#include "Query.h"
#include "RoundConstraint.h"
#include "SoftmaxConstraint.h"
#include "Statistics.h"

class MILPEncoder
{
public:
    MILPEncoder( const ITableau &tableau );

    /*
      Encode the input query as a Gurobi query, variables and inequalities
      are from inputQuery, and latest variable bounds are from tableau
    */
    void encodeQuery( GurobiWrapper &gurobi, const Query &inputQuery, bool relax = false );

    /*
      get variable name from a variable in the encoded inputquery
    */
    String getVariableNameFromVariable( unsigned variable );

    inline void setStatistics( Statistics *statistics )
    {
        _statistics = statistics;
    }

    /*
      Encode the cost function into Gurobi
    */
    void encodeCostFunction( GurobiWrapper &gurobi, const LinearExpression &cost );

private:
    /*
      Tableau has the latest bound
    */
    const ITableau &_tableau;

    /*
      The statistics object.
    */
    Statistics *_statistics;

    /*
      Map the variable to the string encoded in Gurobi
    */
    Map<unsigned, String> _variableToVariableName;

    /*
      Index for Gurobi binary variables
    */
    unsigned _binVarIndex = 0;

    /*
      Index for Gurobi integer variables
    */
    unsigned _intVarIndex = 0;

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
    void encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint *relu, bool relax );

    /*
      Encode a LeakyReLU constraint f = LeakyReLU(b) into Gurobi as a Piecewise Linear Constraint
    */
    void encodeLeakyReLUConstraint( GurobiWrapper &gurobi, LeakyReluConstraint *lRelu, bool relax );

    /*
      Encode a MAX constraint y = max(x_1, x_2, ... ,x_m) into Gurobi using the same encoding in
      https://arxiv.org/pdf/1711.07356.pdf

      Encode the following constraint:

      y <= x_i + (1 - a_i) * (umax - l) & y >= x_i (i = 1 ~ m)
      a_1 + a_2 + ... + a_m = 1
      a_i \in {0, 1} (i = 1 ~ m)
    */
    void encodeMaxConstraint( GurobiWrapper &gurobi, MaxConstraint *max, bool relax );

    /*
      Encode an abs constraint f = Abs(b) into Gurobi
    */
    void encodeAbsoluteValueConstraint( GurobiWrapper &gurobi,
                                        AbsoluteValueConstraint *abs,
                                        bool relax );

    /*
      Encode a sign constraint f = Sign(b) into Gurobi
    */
    void encodeSignConstraint( GurobiWrapper &gurobi, SignConstraint *sign, bool relax );

    /*
      Encode a disjunction constraint into Gurobi
    */
    void
    encodeDisjunctionConstraint( GurobiWrapper &gurobi, DisjunctionConstraint *disj, bool relax );

    /*
      Encode a Sigmoid constraint
    */
    void encodeSigmoidConstraint( GurobiWrapper &gurobi, SigmoidConstraint *sigmoid );

    /*
      Encode a Softmax constraint
    */
    void encodeSoftmaxConstraint( GurobiWrapper &gurobi, SoftmaxConstraint *softmax );

    /*
      Encode a Bilinear constraint
    */
    void
    encodeBilinearConstraint( GurobiWrapper &gurobi, BilinearConstraint *bilinear, bool relax );

    /*
      Encode a Round constraint
    */
    void encodeRoundConstraint( GurobiWrapper &gurobi, RoundConstraint *round, bool relax );
};

#endif // __MILPEncoder_h__
