/*********************                                                        */
/*! \file MILPEncoder.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
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
#include "GurobiWrapper.h"
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
    for ( const auto &plConstraint : inputQuery.getPiecewiseLinearConstraints() )
    {
        switch ( plConstraint->getType() )
        {
        case PiecewiseLinearFunctionType::RELU:
            encodeReLUConstraint( gurobi, (ReluConstraint *)plConstraint );
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

    // Add Transcendental Constraints
    for ( const auto &tsConstraint : inputQuery.getTranscendentalConstraints() )
    {
        switch ( tsConstraint->getType() )
        {
        case TranscendentalFunctionType::SIGMOID:
            encodeSigmoidConstraint( gurobi, (SigmoidConstraint *)tsConstraint );
            break;
        default:
            throw MarabouError( MarabouError::UNSUPPORTED_TRANSCENDENTAL_CONSTRAINT,
                                "GurobiWrapper::encodeInputQuery: "
                                "Only Sigmoid is supported\n" );
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

void MILPEncoder::encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint *relu)
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

    gurobi.addVariable( Stringf( "a%u", _binVarIndex ),
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
    terms.append( GurobiWrapper::Term( -sourceLb, Stringf( "a%u", _binVarIndex ) ) );
    gurobi.addLeqConstraint( terms, -sourceLb );

    terms.clear();
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceUb, Stringf( "a%u", _binVarIndex++ ) ) );
    gurobi.addLeqConstraint( terms, 0 );
}

void MILPEncoder::encodeMaxConstraint( GurobiWrapper &gurobi, MaxConstraint *max )
{
    if ( !max->isActive() )
        return;

    // y = max(x_1, x_2, ... , x_m)
    unsigned y = max->getF();

    // xs = [x_1, x_2, ... , x_m]
    List<unsigned> xs = max->getElements();

    // upper bounds of each x_i
    using qtype = std::pair<double, unsigned>;
    auto cmp = []( qtype l, qtype r) { return l.first <= r.first; };
    std::priority_queue<qtype, std::vector<qtype>, decltype( cmp )> ubq( cmp );

    // terms for Gurobi
    List<GurobiWrapper::Term> terms;

    for ( const auto &x : xs ) 
    {
        // add binary variable
        // Nameing rule is `a{_binVarIndex}_{x}` to clarify
        // which x binary variable is for. 
        gurobi.addVariable( Stringf( "a%u_%u", _binVarIndex, x ),
                            0,
                            1,
                            GurobiWrapper::BINARY );

        terms.append( GurobiWrapper::Term( 1, Stringf( "a%u_%u", _binVarIndex, x ) ) );
        ubq.push( { _tableau.getUpperBound( x ), x } );
    }

    // add constraint: a_1 + a_2 + ... + a_m = 1
    gurobi.addEqConstraint( terms, 1 );

    // extract the pairs of the maximum upper bound and the second.
    auto ubMax1 = ubq.top();
    ubq.pop();
    auto ubMax2 = ubq.top();

    terms.clear();

    double umax = 0;
    for ( const auto &x : xs ) 
    {
        // add constraint: y <= x_i + (1 - a_i) * (umax - l)
        if ( ubMax1.second != x )
            umax = ubMax1.first;
        else
            umax = ubMax2.first;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", y ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", x ) ) );
        terms.append( GurobiWrapper::Term( umax - _tableau.getLowerBound( x ), Stringf( "a%u_%u", _binVarIndex, x ) ) );
        gurobi.addLeqConstraint( terms, umax - _tableau.getLowerBound( x ) );

        terms.clear();
    }
    _binVarIndex++;
}

void MILPEncoder::encodeSigmoidConstraint( GurobiWrapper &gurobi, SigmoidConstraint *sigmoid )
{
    unsigned sourceVariable = sigmoid->getB();  // x_b
    unsigned targetVariable = sigmoid->getF();  // x_f
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double sourceUb = _tableau.getUpperBound( sourceVariable );

    if ( sourceLb == sourceUb )
    {
        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        // In this case, tangentePoint is equal to sourceLb or sourceUb
        double yAtTangentPoint = sigmoid->sigmoid( sourceLb );
        double tangentSlope = sigmoid->sigmoidDerivative( sourceLb );

        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addEqConstraint( terms, -tangentSlope * sourceLb + yAtTangentPoint );
    }
    else if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        List<GurobiWrapper::Term> terms;
        String binVarName = Stringf( "a%u", _binVarIndex ); // a = 1 -> the case where x_b >= 0, otherwise where x_b <= 0
        gurobi.addVariable( binVarName,
                            0,
                            1,
                            GurobiWrapper::BINARY );

        // Constraint where x_b >= 0
        // Upper line is tangent and lower line is secant for an overapproximation with a linearization.

        int binVal = 1;

        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        double tangentPoint = sourceUb / 2;
        double yAtTangentPoint = sigmoid->sigmoid( tangentPoint );
        double tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        terms.clear();

        // secant line: x_f = secantSlope * (x_b - 0) + y_l
        double y_l = sigmoid->sigmoid( 0 );
        double y_u = sigmoid->sigmoid( sourceUb );
        double secantSlope = ( y_u - y_l ) / sourceUb;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -secantSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, y_l );
        terms.clear();

        // lower bound of x_b
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, 0 );  
        terms.clear();

        // lower bound of x_f
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, y_l );  
        terms.clear(); 

        // Constraints where x_b <= 0
        // Upper line is secant and lower line is tangent for an overapproximation with a linearization.

        binVal = 0;

        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        tangentPoint = sourceLb / 2;
        yAtTangentPoint = sigmoid->sigmoid( tangentPoint );
        tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        terms.clear();

        // secant line: x_f = secantSlope * (x_b - sourceLb) + y_l
        y_u = y_l;
        y_l = sigmoid->sigmoid( sourceLb );
        secantSlope = ( y_u - y_l ) / ( 0 - sourceLb );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -secantSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, -secantSlope * sourceLb + y_l );
        terms.clear();

        // upper bound of x_b
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, 0 );  
        terms.clear();

        // upper bound of x_f
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, y_u );  
        terms.clear(); 

        _binVarIndex++;
    }
    else
    {   
        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        double tangentPoint = ( sourceLb + sourceUb ) / 2;
        double yAtTangentPoint = sigmoid->sigmoid( tangentPoint );
        double tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );

        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );

        if ( FloatUtils::gte( sourceLb, 0 ) )
        {
            gurobi.addLeqConstraint( terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        }
        else
        {
            gurobi.addGeqConstraint( terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        }
        terms.clear();

        double y_l = sigmoid->sigmoid( sourceLb );
        double y_u = sigmoid->sigmoid( sourceUb );

        double secantSlope = ( y_u - y_l ) / ( sourceUb - sourceLb );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -secantSlope, Stringf( "x%u", sourceVariable ) ) );

        if ( FloatUtils::gte( sourceLb, 0 ) )
        {
            gurobi.addGeqConstraint( terms, -secantSlope * sourceLb + y_l );
        }
        else
        {
            gurobi.addLeqConstraint( terms, -secantSlope * sourceLb + y_l );
        }
        terms.clear();
    }
}