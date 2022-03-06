/*********************                                                        */
/*! \file PlcExplanation.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __PlcExplanation_h__
#define __PlcExplanation_h__

#include "PiecewiseLinearConstraint.h"

enum BoundType : bool
{
    UPPER = true,
    LOWER = false,
};

/*
  Contains all necessary info of a ground bound update during a run (i.e from ReLU phase-fixing)
*/
class PLCExplanation
{
public:
    PLCExplanation( unsigned causingVar, unsigned affectedVar, double bound, BoundType causingVarBound, BoundType affectedVarBound, double *explanation, PiecewiseLinearFunctionType constraintType, unsigned decisionLevel );
    ~PLCExplanation();

    unsigned getDecisionLevel();

private:
    unsigned _causingVar;
    unsigned _affectedVar;
    double _bound;
    BoundType _causingVarBound;
    BoundType _affectedVarBound;
    double *_explanation;
    PiecewiseLinearFunctionType _constraintType;
    unsigned _decisionLevel;

friend class UnsatCertificateNode;
};

#endif //__PlcExplanation_h__