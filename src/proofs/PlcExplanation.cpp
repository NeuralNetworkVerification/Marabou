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

PLCExplanation::PLCExplanation( unsigned causingVar,
                                unsigned affectedVar,
                                double bound,
                                BoundType causingVarBound,
                                BoundType affectedVarBound,
                                const Vector<double> &explanation,
                                PiecewiseLinearFunctionType constraintType )
    : _causingVar( causingVar )
    , _affectedVar( affectedVar )
    , _bound( bound )
    , _causingVarBound( causingVarBound )
    , _affectedVarBound( affectedVarBound )
    , _constraintType( constraintType )
{
    if ( explanation.empty() )
        _explanation = NULL;
    else
    {
        _explanation = new double[explanation.size()];
        std::copy( explanation.begin(), explanation.end(), _explanation );
    }
}

PLCExplanation::~PLCExplanation()
{
    if ( _explanation )
    {
        delete [] _explanation;
        _explanation = NULL;
    }
}

unsigned PLCExplanation::getCausingVar() const
{
    return _causingVar;
}

unsigned PLCExplanation::getAffectedVar() const
{
    return _affectedVar;
}

double PLCExplanation::getBound() const
{
    return _bound;
}

BoundType PLCExplanation::getCausingVarBound() const
{
    return _causingVarBound;
}

BoundType PLCExplanation::getAffectedVarBound() const
{
    return _affectedVarBound;
}

const double *PLCExplanation::getExplanation() const
{
    return _explanation;
}

PiecewiseLinearFunctionType PLCExplanation::getConstraintType() const
{
    return _constraintType;
}
