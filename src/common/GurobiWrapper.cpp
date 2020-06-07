/*********************                                                        */
/*! \file GurobiWrapper.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifdef ENABLE_GUROBI

#include "Debug.h"
#include "GurobiWrapper.h"

#include <iostream>

using namespace std;

GurobiWrapper::GurobiWrapper()
    : _environment( NULL )
    , _model( NULL )
{
    _environment = new GRBEnv;
    _model = new GRBModel( *_environment );
}

GurobiWrapper::~GurobiWrapper()
{
    freeMemoryIfNeeded();
}

void GurobiWrapper::freeMemoryIfNeeded()
{
    for ( auto &entry : _nameToVariable )
    {
        delete entry.second;
        entry.second = NULL;
    }
    _nameToVariable.clear();

    if ( _model )
    {
        delete _model;
        _model = NULL;
    }

    if ( _environment )
    {
        delete _environment;
        _environment = NULL;
    }
}

void GurobiWrapper::reset()
{
    freeMemoryIfNeeded();
    _environment = new GRBEnv;
    _model = new GRBModel( *_environment );
}

void GurobiWrapper::addVariable( String name, double lb, double ub )
{
    ASSERT( !_nameToVariable.exists( name ) );

    GRBVar *newVar = new GRBVar;
    double objectiveValue = 0;
    *newVar = _model->addVar( lb,
                              ub,
                              objectiveValue,
                              GRB_CONTINUOUS,
                              name.ascii() );


    _nameToVariable[name] = newVar;
}

void GurobiWrapper::addLeqConstraint( const List<Term> &terms, double scalar )
{
    addConstraint( terms, scalar, GRB_LESS_EQUAL );
}

void GurobiWrapper::addGeqConstraint( const List<Term> &terms, double scalar )
{
    addConstraint( terms, scalar, GRB_GREATER_EQUAL );
}

void GurobiWrapper::addEqConstraint( const List<Term> &terms, double scalar )
{
    addConstraint( terms, scalar, GRB_EQUAL );
}

void GurobiWrapper::addConstraint( const List<Term> &terms, double scalar, char sense )
{
    GRBLinExpr constraint;

    for ( const auto &term : terms )
    {
        ASSERT( _nameToVariable.exists( term._variable ) );
        constraint += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
    }

    _model->addConstr( constraint, sense, scalar );
}

void GurobiWrapper::setCost( const List<Term> &terms )
{
    GRBLinExpr cost;

    for ( const auto &term : terms )
    {
        ASSERT( _nameToVariable.exists( term._variable ) );
        cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
    }

    _model->setObjective( cost, GRB_MINIMIZE );
}

void GurobiWrapper::setObjective( const List<Term> &terms )
{
    GRBLinExpr cost;

    for ( const auto &term : terms )
    {
        ASSERT( _nameToVariable.exists( term._variable ) );
        cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
    }

    _model->setObjective( cost, GRB_MAXIMIZE );
}

void GurobiWrapper::solve()
{
    _model->optimize();
}

void GurobiWrapper::extractSolution( Map<String, double> &values, double &cost )
{
    values.clear();

    for ( const auto &variable : _nameToVariable )
        values[variable.first] = variable.second->get( GRB_DoubleAttr_X );

    cost = _model->get( GRB_DoubleAttr_ObjVal );
}

#endif // ENABLE_GUROBI

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
