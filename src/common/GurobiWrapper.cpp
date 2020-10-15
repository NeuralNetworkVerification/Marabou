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
#include "Equation.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "MStringf.h"
#include "Options.h"
#include "PiecewiseLinearConstraint.h"
#include "ReluConstraint.h"

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
    _model->getEnv().set( GRB_IntParam_Threads,
                          GlobalConfiguration::GUROBI_NUMBER_OF_THREADS );

    // Timeout
    setTimeLimit( _timeoutInSeconds );
}

void GurobiWrapper::reset()
{
    _model->reset();
    _variableToVariableName.clear();
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

void GurobiWrapper::setTimeLimit( double seconds )
{
    _model->set( GRB_DoubleParam_TimeLimit, seconds );
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
    return _model->get( GRB_IntAttr_Status ) == GRB_OPTIMAL;
}

bool GurobiWrapper::cutoffOccurred()
{
    return _model->get( GRB_IntAttr_Status ) == GRB_CUTOFF;
}

bool GurobiWrapper::infeasbile()
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
                                    e.getMessage().c_str() ).ascii() );
    }
}

double GurobiWrapper::getObjectiveBound()
{
    return _model->get( GRB_DoubleAttr_ObjBound );
}

void GurobiWrapper::dumpModel( String name )
{
    _model->write( name.ascii() );
}

String GurobiWrapper::getVariableNameFromVariable( unsigned variable )
{
    if ( !_variableToVariableName.exists( variable ) )
        throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );
    return _variableToVariableName[variable];
}

void GurobiWrapper::encodeInputQuery( const InputQuery &inputQuery )
{
    reset();
    for ( unsigned var = 0; var < inputQuery.getNumberOfVariables(); var++ )
    {
        double lb = inputQuery.getLowerBound( var );
        double ub = inputQuery.getUpperBound( var );
        String varName = Stringf( "x%u", var );
        addVariable( varName, lb, ub );
        _variableToVariableName[var] = varName;
    }
    for ( const auto &eq : inputQuery.getEquations() )
    {
        List<Term> terms;
        double scalar = eq._scalar;
        for ( const auto term : eq._addends )
            terms.append( Term( term._coefficient, Stringf( "x%u", term._variable ) ) );
        switch ( eq._type )
        {
        case Equation::EQ:
            addEqConstraint( terms, scalar );
            break;
        case Equation::LE:
            addLeqConstraint( terms, scalar );
            break;
        case Equation::GE:
            addGeqConstraint( terms, scalar );
            break;
        default:
            break;
        }
    }

    unsigned ind = 0;
    for ( const auto&plConstraint : inputQuery.getPiecewiseLinearConstraints() )
    {
        if ( !plConstraint->isActive() || plConstraint->phaseFixed() )
            continue;
        if ( plConstraint->getType() != PiecewiseLinearFunctionType::RELU )
            throw MarabouError( MarabouError::UNSUPPORTED_PIECEWISE_CONSTRAINT,
                                "GurobiWrapper::encodeInputQuery: "
                                "Only ReLU is supported\n" );

        auto relu = ( ReluConstraint *) plConstraint;
        addVariable( Stringf( "a%u", ind ),
                     0,
                     1,
                     GurobiWrapper::BINARY );

        unsigned sourceVariable = relu->getB();
        unsigned targetVariable = relu->getF();
        double sourceLb = inputQuery.getLowerBound( sourceVariable );
        double sourceUb = inputQuery.getUpperBound( sourceVariable );

        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
        terms.append( GurobiWrapper::Term( -sourceLb, Stringf( "a%u", ind ) ) );
        addLeqConstraint( terms, -sourceLb );

        terms.clear();
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -sourceUb, Stringf( "a%u", ind ) ) );
        addLeqConstraint( terms, 0 );
        ind++;
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
