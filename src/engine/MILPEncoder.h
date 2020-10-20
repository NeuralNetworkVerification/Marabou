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
      Encode a ReLU constraint into Gurobi
      Encode the following constraint:

    */
    void encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint *relu,
                               unsigned index );

};

#endif // __MILPEncoder_h__
