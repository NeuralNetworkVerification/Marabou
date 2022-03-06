/*********************                                                        */
/*! \file PlcExplanation.cpp
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

#include "PlcExplanation.h"

PLCExplanation::PLCExplanation( unsigned causingVar, unsigned affectedVar, double bound, BoundType causingVarBound, BoundType affectedVarBound, double *explanation, PiecewiseLinearFunctionType constraintType, unsigned decisionLevel )
        :_causingVar( causingVar )
        ,_affectedVar( affectedVar )
        ,_bound( bound )
        ,_causingVarBound( causingVarBound )
        ,_affectedVarBound( affectedVarBound )
        ,_explanation( explanation )
        ,_constraintType( constraintType )
        ,_decisionLevel( decisionLevel )
{
}

PLCExplanation::~PLCExplanation()
{
    if ( _explanation )
    {
        delete [] _explanation;
        _explanation = NULL;
    }
}

unsigned PLCExplanation::getDecisionLevel()
{
    return _decisionLevel;
}