/*********************                                                        */
/*! \file GurobiWrapper.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifdef ENABLE_GUROBI

#include "GurobiWrapper.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "Options.h"
#include "gurobi_c.h"

#include <iostream>

using namespace std;

GurobiWrapper::GurobiWrapper()
    : _environment( NULL )
    , _model( NULL )
    , _timeoutInSeconds( Options::get()->getFloat( Options::MILP_SOLVER_TIMEOUT ) )
{
    _environment = new GRBEnv;
    resetModel();
}

GurobiWrapper::~GurobiWrapper()
{
    freeMemoryIfNeeded();
}

void GurobiWrapper::freeModelIfNeeded()
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
}

void GurobiWrapper::freeMemoryIfNeeded()
{
    freeModelIfNeeded();

    if ( _environment )
    {
        delete _environment;
        _environment = NULL;
    }
}

void GurobiWrapper::resetModel()
{
    freeModelIfNeeded();
    _model = new GRBModel( *_environment );

    // Suppress printing
    _model->getEnv().set( GRB_IntParam_OutputFlag, 0 );

    // Thread number
    _model->getEnv().set( GRB_IntParam_Threads, GlobalConfiguration::GUROBI_NUMBER_OF_THREADS );

    // Numeral parameters
    _model->getEnv().set( GRB_DoubleParam_FeasibilityTol,
                          std::max( GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS, 1e-9 ) );

    _model->getEnv().set( GRB_DoubleParam_IntFeasTol,
                          std::max( GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE, 1e-8 ) );

    // Timeout
    setTimeLimit( _timeoutInSeconds );
}

void GurobiWrapper::reset()
{
    _model->reset();
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

    case INTEGER:
        variableType = GRB_INTEGER;
        break;

    default:
        break;
    }

    try
    {
        GRBVar *newVar = new GRBVar;
        double objectiveValue = 0;
        *newVar = _model->addVar( lb, ub, objectiveValue, variableType, name.ascii() );

        _nameToVariable[name] = newVar;
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::setLowerBound( String name, double lb )
{
    GRBVar var = _model->getVarByName( name.ascii() );
    var.set( GRB_DoubleAttr_LB, lb );
}

void GurobiWrapper::setUpperBound( String name, double ub )
{
    GRBVar var = _model->getVarByName( name.ascii() );
    var.set( GRB_DoubleAttr_UB, ub );
}

void GurobiWrapper::setCutoff( double cutoff )
{
    _model->set( GRB_DoubleParam_Cutoff, cutoff );
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
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::addPiecewiseLinearConstraint( String sourceVariable,
                                                  String targetVariable,
                                                  unsigned numPoints,
                                                  const double *xPoints,
                                                  const double *yPoints )
{
    try
    {
        _model->addGenConstrPWL( *_nameToVariable[sourceVariable],
                                 *_nameToVariable[targetVariable],
                                 numPoints,
                                 xPoints,
                                 yPoints );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::addLeqIndicatorConstraint( const String binVarName,
                                               const int binVal,
                                               const List<Term> &terms,
                                               double scalar )
{
    addIndicatorConstraint( binVarName, binVal, terms, scalar, GRB_LESS_EQUAL );
}

void GurobiWrapper::addGeqIndicatorConstraint( const String binVarName,
                                               const int binVal,
                                               const List<Term> &terms,
                                               double scalar )
{
    addIndicatorConstraint( binVarName, binVal, terms, scalar, GRB_GREATER_EQUAL );
}

void GurobiWrapper::addEqIndicatorConstraint( const String binVarName,
                                              const int binVal,
                                              const List<Term> &terms,
                                              double scalar )
{
    addIndicatorConstraint( binVarName, binVal, terms, scalar, GRB_EQUAL );
}

void GurobiWrapper::addIndicatorConstraint( const String binVarName,
                                            const int binVal,
                                            const List<Term> &terms,
                                            double scalar,
                                            char sense )
{
    try
    {
        GRBLinExpr constraint;

        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            constraint += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        ASSERT( _nameToVariable.exists( binVarName ) );
        _model->addGenConstrIndicator(
            *_nameToVariable[binVarName], binVal, constraint, sense, scalar );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::addBilinearConstraint( const String input1,
                                           const String input2,
                                           const String output )
{
    _model->addQConstr( ( *_nameToVariable[output] ) ==
                        ( *_nameToVariable[input1] ) * ( *_nameToVariable[input2] ) );
}

void GurobiWrapper::setCost( const List<Term> &terms, double constant )
{
    try
    {
        GRBLinExpr cost;

        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        cost += constant;

        _model->setObjective( cost, GRB_MINIMIZE );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::setObjective( const List<Term> &terms, double constant )
{
    try
    {
        GRBLinExpr cost;

        for ( const auto &term : terms )
        {
            ASSERT( _nameToVariable.exists( term._variable ) );
            cost += GRBLinExpr( *_nameToVariable[term._variable], term._coefficient );
        }

        cost += constant;

        _model->setObjective( cost, GRB_MAXIMIZE );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::setTimeLimit( double seconds )
{
    _model->set( GRB_DoubleParam_TimeLimit, seconds );
}

void GurobiWrapper::solve()
{
    try
    {
        _model->optimize();
        log( Stringf( "Model status: %u\n", _model->get( GRB_IntAttr_Status ) ) );
    }
    catch ( GRBException e )
    {
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

bool GurobiWrapper::optimal()
{
    return _model->get( GRB_IntAttr_Status ) == GRB_OPTIMAL;
}

bool GurobiWrapper::cutoffOccurred()
{
    return _model->get( GRB_IntAttr_Status ) == GRB_CUTOFF;
}

bool GurobiWrapper::infeasible()
{
    return _model->get( GRB_IntAttr_Status ) == GRB_INFEASIBLE;
}

bool GurobiWrapper::timeout()
{
    return _model->get( GRB_IntAttr_Status ) == GRB_TIME_LIMIT;
}

bool GurobiWrapper::haveFeasibleSolution()
{
    return _model->get( GRB_IntAttr_SolCount ) > 0;
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
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

double GurobiWrapper::getObjectiveBound()
{
    try
    {
        return _model->get( GRB_DoubleAttr_ObjBound );
    }
    catch ( GRBException e )
    {
        log( "Failed to get objective bound from Gurobi." );
        if ( e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE )
        {
            // From https://www.gurobi.com/documentation/9.0/refman/py_model_setattr.html
            // due to our lazy update approach, the change won't actually take effect until you
            // update the model (using Model.update), optimize the model (using Model.optimize),
            // or write the model to disk (using Model.write).
            _model->update();

            if ( _model->get( GRB_IntAttr_ModelSense ) == 1 )
                // case minimize
                return FloatUtils::negativeInfinity();
            else
                // case maximize
                return FloatUtils::infinity();
        }
        throw CommonError( CommonError::GUROBI_EXCEPTION,
                           Stringf( "Gurobi exception. Gurobi Code: %u, message: %s\n",
                                    e.getErrorCode(),
                                    e.getMessage().c_str() )
                               .ascii() );
    }
}

void GurobiWrapper::dumpModel( String name )
{
    _model->write( name.ascii() );
}

void GurobiWrapper::log( const String &message )
{
    if ( GlobalConfiguration::GUROBI_LOGGING )
        printf( "GurobiWrapper: %s\n", message.ascii() );
}

#endif // ENABLE_GUROBI

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
