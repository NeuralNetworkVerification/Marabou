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
#include "MStringf.h"
#include "gurobi_c.h"

#include <iostream>

using namespace std;

GurobiWrapper::GurobiWrapper()
    : _environment( NULL )
    , _model( NULL )
{
    _environment = new GRBEnv;
    _model = new GRBModel( *_environment );

    // Suppress printing
    _model->getEnv().set( GRB_IntParam_OutputFlag, 0 );
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

void GurobiWrapper::addVariable( String name, double lb, double ub, VariableType type )
{
    ASSERT( !_nameToVariable.exists( name ) );

    char variableType = GRB_CONTINUOUS;
    switch ( type )
    {
    case CONTINUOUS:
        variableType = GRB_CONTINUOUS;
        break;

    case BINARY:
        variableType = GRB_BINARY;
        break;

    default:
        break;
    }

    try
    {
        GRBVar *newVar = new GRBVar;
        double objectiveValue = 0;
        *newVar = _model->addVar( lb,
                                  ub,
                                  objectiveValue,
                                  variableType,
                                  name.ascii() );

        _nameToVariable[name] = newVar;
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
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
    try
    {
        GRBLinExpr constraint;


        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            constraint += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        _model->addConstr( constraint, sense, scalar );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
}

void GurobiWrapper::setCost( const List<Term> &terms )
{
    try
    {
        GRBLinExpr cost;

        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        _model->setObjective( cost, GRB_MINIMIZE );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
}

void GurobiWrapper::setObjective( const List<Term> &terms )
{
    try
    {
        GRBLinExpr cost;

        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        _model->setObjective( cost, GRB_MAXIMIZE );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
}

void GurobiWrapper::solve()
{
    try
    {
        _model->optimize();
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
}

bool GurobiWrapper::optimal()
{
    return ( _model->get( GRB_IntAttr_Status ) == GRB_OPTIMAL );
}

bool GurobiWrapper::infeasbile()
{
    return ( _model->get( GRB_IntAttr_Status ) == GRB_INFEASIBLE );
}

void GurobiWrapper::extractSolution( Map<String, double> &values, double &costOrObjective )
{
    try
    {
        values.clear();

        for ( const auto &variable : _nameToVariable )
            values[variable.first] = variable.second->get( GRB_DoubleAttr_X );

        costOrObjective = _model->get( GRB_DoubleAttr_ObjVal );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() ).ascii() );
    }
}

#endif // ENABLE_GUROBI

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
