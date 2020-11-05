/*********************                                                        */
/*! \file MILPEncoder.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "FloatUtils.h"
#include "MILPEncoder.h"

MILPEncoder::MILPEncoder( const ITableau &tableau )
    : _tableau( tableau )
{}

void MILPEncoder::encodeInputQuery( GurobiWrapper &gurobi,
                                    const InputQuery &inputQuery )
{
    gurobi.reset();
    // Add variables
    for ( unsigned var = 0; var < inputQuery.getNumberOfVariables(); var++ )
    {
        double lb = _tableau.getLowerBound( var );
        double ub = _tableau.getUpperBound( var );
        String varName = Stringf( "x%u", var );
        gurobi.addVariable( varName, lb, ub );
        _variableToVariableName[var] = varName;
    }

    // Add equations
    for ( const auto &equation : inputQuery.getEquations() )
    {
        encodeEquation( gurobi, equation );
    }

    // Add Piecewise-linear Constraints
    unsigned ind = 0;
    for ( const auto &plConstraint : inputQuery.getPiecewiseLinearConstraints() )
    {
        switch ( plConstraint->getType() )
        {
        case PiecewiseLinearFunctionType::RELU:
            encodeReLUConstraint( gurobi, (ReluConstraint *)plConstraint,
                                  ind++ );
            break;
        case PiecewiseLinearFunctionType::MAX:
            encodeMaxConstraint( gurobi, (MaxConstraint *)plConstraint );
            break;
        default:
            throw MarabouError( MarabouError::UNSUPPORTED_PIECEWISE_LINEAR_CONSTRAINT,
                                "GurobiWrapper::encodeInputQuery: "
                                "Only ReLU and Max are supported\n" );
        }
    }
}

String MILPEncoder::getVariableNameFromVariable( unsigned variable )
{
    if ( !_variableToVariableName.exists( variable ) )
        throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );
    return _variableToVariableName[variable];
}

void MILPEncoder::encodeEquation( GurobiWrapper &gurobi, const Equation &equation )
{
    List<GurobiWrapper::Term> terms;
    double scalar = equation._scalar;
    for ( const auto &term : equation._addends )
        terms.append( GurobiWrapper::Term
                      ( term._coefficient,
                        Stringf( "x%u", term._variable ) ) );
    switch ( equation._type )
    {
    case Equation::EQ:
        gurobi.addEqConstraint( terms, scalar );
        break;
    case Equation::LE:
        gurobi.addLeqConstraint( terms, scalar );
        break;
    case Equation::GE:
        gurobi.addGeqConstraint( terms, scalar );
        break;
    default:
        break;
    }
}

void MILPEncoder::encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint
                                        *relu, unsigned index )
{

    if ( !relu->isActive() || relu->phaseFixed() )
    {
        ASSERT( relu->auxVariableInUse() );
        ASSERT( ( FloatUtils::gte( _tableau.getLowerBound( relu->getB() ),  0 ) &&
                  FloatUtils::lte( _tableau.getLowerBound( relu->getAux() ), 0 ) )
                ||
                ( FloatUtils::lte( _tableau.getUpperBound( relu->getB() ), 0 ) &&
                  FloatUtils::lte( _tableau.getUpperBound( relu->getF() ), 0 ) ) );
        return;
    }

    gurobi.addVariable( Stringf( "a%u", index ),
                        0,
                        1,
                        GurobiWrapper::BINARY );

    unsigned sourceVariable = relu->getB();
    unsigned targetVariable = relu->getF();
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double sourceUb = _tableau.getUpperBound( sourceVariable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceLb, Stringf( "a%u", index ) ) );
    gurobi.addLeqConstraint( terms, -sourceLb );

    terms.clear();
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceUb, Stringf( "a%u", index ) ) );
    gurobi.addLeqConstraint( terms, 0 );
}

void MILPEncoder::encodeMaxConstraint( GurobiWrapper &gurobi, MaxConstraint *max )
{

    if ( !max->isActive() )
        return;

    // y = max(x_1, x_2, ... , x_m)
    unsigned y = max->getF();

    // xs = [x_1, x_2, ... , x_m]
    List<unsigned> xs = max->getParticipatingVariables();
    unsigned m = xs.size();

    // upper bounds of each x_i
    double* ubs = new double[m];

    // terms for Gurobi
    List<GurobiWrapper::Term> terms;

    unsigned i = 0;
    for ( const auto &x : xs ) {
        // add binary variable
        gurobi.addVariable( Stringf( "a%u", x ),
                            0,
                            1,
                            GurobiWrapper::BINARY );

        terms.append( GurobiWrapper::Term( 1, Stringf( "a%u", x ) ) );
        ubs[i] = _tableau.getUpperBound( x );
        i++;
    }

    // add constraint: a_1 + a_2 + ... + a_m = 1
    gurobi.addEqConstraint( terms, 1 );

    terms.clear();

    i = 0;
    for ( const auto &x : xs ) {
        // add constraint: y <= x_i + (1 - a_i) * (umax - l)
        double umax = getUmax(ubs, i, m);
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", y ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", x ) ) );
        terms.append( GurobiWrapper::Term( umax - _tableau.getLowerBound( x ), Stringf( "a%u", x ) ) );
        gurobi.addLeqConstraint( terms, umax - _tableau.getLowerBound( x ) );

        terms.clear();

        // add constraint: y >= x_i
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", y ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", x ) ) );
        gurobi.addGeqConstraint( terms, 0);

        terms.clear();
        
        i++;
    }
}

double MILPEncoder::getUmax( const double *ubs, int j,
                              int m)
{
    int umaxIndex = -1;
    
    for (int i = 0; i < m; i++) {
        if (i == j) {
            continue;
        }

        if (umaxIndex == -1) {
            umaxIndex = i;
            continue;
        }

        if (ubs[umaxIndex] < ubs[i]) {
            umaxIndex = i;
        }
    }

    return ubs[umaxIndex];  
}