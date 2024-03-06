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
                    Tightening::BoundType causingVarBound,
                    Tightening::BoundType affectedVarBound,
                    const Vector<SparseUnsortedList> &explanations,
                    PiecewiseLinearFunctionType constraintType )
    : _causingVars( causingVars )
    , _affectedVar( affectedVar )
    , _bound( bound )
    , _causingVarBound( causingVarBound )
    , _affectedVarBound( affectedVarBound )
    , _constraintType( constraintType )
{
    if ( explanations.empty() )
        _explanations = List<SparseUnsortedList>();
    else
    {
        ASSERT( causingVars.size() == explanations.size() );

        unsigned numOfExplanations = explanations.size();

        ASSERT( numOfExplanations == causingVars.size() );

        if ( _constraintType == RELU || _constraintType == SIGN )
            ASSERT( numOfExplanations == 1 );

        if ( _constraintType == ABSOLUTE_VALUE )
            ASSERT( numOfExplanations == 2 || numOfExplanations == 1 );

        _explanations = List<SparseUnsortedList>();

        for ( unsigned i = 0; i < numOfExplanations; ++i )
        {
            SparseUnsortedList expl = SparseUnsortedList();
            expl = explanations[i];
            _explanations.append( expl );
        }
    }
}

PLCLemma::~PLCLemma()
{
    if ( !_explanations.empty() )
        for ( auto &expl : _explanations )
            expl.clear();
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

Tightening::BoundType PLCLemma::getCausingVarBound() const
{
    return _causingVarBound;
}

Tightening::BoundType PLCLemma::getAffectedVarBound() const
{
    return _affectedVarBound;
}

const List<SparseUnsortedList> &PLCLemma::getExplanations() const
{
    return _explanations;
}

PiecewiseLinearFunctionType PLCLemma::getConstraintType() const
{
    return _constraintType;
}
