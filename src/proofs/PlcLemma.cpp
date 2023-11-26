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

#include "PlcLemma.h"

PLCLemma::PLCLemma( const List<unsigned> &causingVars,
                    unsigned affectedVar,
                    double bound,
                    BoundType causingVarBound,
                    BoundType affectedVarBound,
                    const Vector<Vector<double>> &explanations,
                    PiecewiseLinearFunctionType constraintType )
    : _causingVars( causingVars )
    , _affectedVar( affectedVar )
    , _bound( bound )
    , _causingVarBound( causingVarBound )
    , _affectedVarBound( affectedVarBound )
    , _constraintType( constraintType )
{

    if ( explanations.empty() )
        _explanations = NULL;
    else
    {
        bool allEmpty = true;
        unsigned proofSize = 0;

        for ( const auto &expl : explanations )
            if ( !expl.empty() )
            {
                proofSize = expl.size();
                allEmpty = false;
                break;
            }

        if ( allEmpty )
            _explanations = NULL;
        else
        {
            unsigned numOfExplanations = explanations.size();

            ASSERT( numOfExplanations == causingVars.size() && proofSize );

            if ( _constraintType == RELU || _constraintType == SIGN )
            {
                ASSERT( numOfExplanations == 1 );
            }

            if ( _constraintType == ABSOLUTE_VALUE )
            {
                ASSERT( numOfExplanations == 2 || numOfExplanations == 1 );
            }

            _explanations = new double[numOfExplanations * proofSize];

            for ( unsigned i = 0; i < numOfExplanations; ++i )
                for ( unsigned j = 0; j < proofSize; ++j )
                    _explanations[i * proofSize + j] = explanations[i][j];

        }
    }
}

PLCLemma::~PLCLemma()
{
    if ( _explanations )
    {
        delete [] _explanations;
        _explanations = NULL;
    }
}

const List<unsigned> &PLCLemma::getCausingVars() const
{
    return _causingVars;
}

unsigned PLCLemma::getAffectedVar() const
{
    return _affectedVar;
}

double PLCLemma::getBound() const
{
    return _bound;
}

BoundType PLCLemma::getCausingVarBound() const
{
    return _causingVarBound;
}

BoundType PLCLemma::getAffectedVarBound() const
{
    return _affectedVarBound;
}

const double *PLCLemma::getExplanations() const
{
    return _explanations;
}

PiecewiseLinearFunctionType PLCLemma::getConstraintType() const
{
    return _constraintType;
}
