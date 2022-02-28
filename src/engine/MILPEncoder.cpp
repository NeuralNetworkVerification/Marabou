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
        addTangentLineOnSigmoid( gurobi, sigmoid, sourceLb, yAtTangentPoint, sourceLb, sourceUb );
    }
    else if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        // set binVarName
        String binVarName = Stringf( "a%u", _binVarIndex ); // a = 1 -> the case where x_b >= 0, otherwise where x_b <= 0
        gurobi.addVariable( binVarName,
                            0,
                            1,
                            GurobiWrapper::BINARY );
        _binVarIndex++;
        sigmoid->setBinVarName( binVarName );

        int binVal = 1;

        List<GurobiWrapper::Term> terms;

        // Constraint where x_b >= 0
        // Upper line is tangent and lower line is secant for an overapproximation with a linearization.

        // add a tangent line
        double xpt = sourceUb / 2;
        double ypt = sigmoid->sigmoid( xpt );
        addTangentLineOnSigmoid( gurobi, sigmoid, xpt, ypt, sourceLb, sourceUb );

        // add a split point
        sigmoid->addSplitPoint( xpt, ypt );

        // add secant lines
        double y_l = sigmoid->sigmoid( 0 );
        double y_u = sigmoid->sigmoid( sourceUb );
        double xpts[3] = { 0, xpt, sourceUb };
        double ypts[3] = { y_l, ypt, y_u };
        addSecantLinesOnSigmoid( gurobi, sigmoid, xpt, 3, xpts, ypts, sourceLb, sourceUb );

        // add split points
        sigmoid->addSplitPoint( sourceUb, y_u );
        sigmoid->addSplitPoint( 0, y_l );

        // set lower bound of x_b
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, 0 );  
        terms.clear();

        // set lower bound of x_f
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        gurobi.addGeqIndicatorConstraint( binVarName, binVal, terms, y_l );  
        terms.clear(); 

        // Constraints where x_b <= 0
        // Upper line is secant and lower line is tangent for an overapproximation with a linearization.

        binVal = 0;

        // add a tangent line
        xpt = sourceLb / 2;
        ypt = sigmoid->sigmoid( xpt );
        addTangentLineOnSigmoid( gurobi, sigmoid, xpt, ypt, sourceLb, sourceUb );

        // add a split point
        sigmoid->addSplitPoint( xpt, ypt );

        // add secant lines
        y_u = y_l;
        y_l = sigmoid->sigmoid( sourceLb );
        double xptsNeg[3] = { sourceLb, xpt, 0 };
        double yptsNeg[3] = { y_l, ypt, y_u };
        addSecantLinesOnSigmoid( gurobi, sigmoid, xpt, 3, xptsNeg, yptsNeg, sourceLb, sourceUb );

        // add a split point
        sigmoid->addSplitPoint( sourceLb, y_l );

        // upper bound of x_b
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, 0 );  
        terms.clear();

        // upper bound of x_f
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        gurobi.addLeqIndicatorConstraint( binVarName, binVal, terms, y_u );  
        terms.clear();
    }
    else
    {   
        // add a tangent line
        double xpt = ( sourceLb + sourceUb ) / 2;
        double ypt = sigmoid->sigmoid( xpt );
        addTangentLineOnSigmoid( gurobi, sigmoid, xpt, ypt, sourceLb, sourceUb );

        // add a split point
        sigmoid->addSplitPoint( xpt, ypt );
        
        double y_l = sigmoid->sigmoid( sourceLb );
        double y_u = sigmoid->sigmoid( sourceUb );

        // add split points
        sigmoid->addSplitPoint( sourceLb, y_l );
        sigmoid->addSplitPoint( sourceUb, y_u );

        double xpts[3] = { sourceLb, xpt, sourceUb };
        double ypts[3] = { y_l, ypt, y_u };

        // add secant lines
        addSecantLinesOnSigmoid( gurobi, sigmoid, xpt, 3, xpts, ypts, sourceLb, sourceUb ); 
    }
}

void MILPEncoder::addTangentLineOnSigmoid( GurobiWrapper &gurobi, SigmoidConstraint *sigmoid, double tangentPoint, double yAtTangentPoint, double sourceLb, double sourceUb )
{
    unsigned sourceVariable = sigmoid->getB();  // x_b
    unsigned targetVariable = sigmoid->getF();  // x_f
    double tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );

    // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );

    if ( sourceLb == sourceUb )
        // In this case, tangentePoint is equal to sourceLb or sourceUb
        gurobi.addEqConstraint( terms, -tangentSlope * sourceLb + yAtTangentPoint );
    else if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        String binVarName = sigmoid->getBinVarName();
        ASSERT( binVarName != "" );
        
        if ( FloatUtils::gte( tangentPoint, 0 ) )
            gurobi.addLeqIndicatorConstraint( binVarName, 1, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        else
            gurobi.addGeqIndicatorConstraint( binVarName, 0, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
    }
    else
    {
        if ( FloatUtils::gte( tangentPoint, 0 ) )
            gurobi.addLeqConstraint( terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        else
            gurobi.addGeqConstraint( terms, -tangentSlope * tangentPoint + yAtTangentPoint );
    }
}

void MILPEncoder::addSecantLinesOnSigmoid( GurobiWrapper &gurobi, SigmoidConstraint *sigmoid, 
                                            double newSplitPoint, unsigned numOfPts, double *xpts, 
                                            double *ypts, double sourceLb, double sourceUb )
{
    unsigned sourceVariable = sigmoid->getB();  // x_b
    unsigned targetVariable = sigmoid->getF();  // x_f

    // get var names in gurobi
    String xVarName = getVariableNameFromVariable( sourceVariable );
    String yVarName = getVariableNameFromVariable( targetVariable );

    List<GurobiWrapper::Term> terms;

    // add a piece-wise linear constraint
    String yplVarName = Stringf( "pl_%s_%u", yVarName.ascii(), _plVarIndex );
    gurobi.addVariable( yplVarName, ypts[0], ypts[numOfPts - 1] );
    _plVarIndex++;
    gurobi.addPiecewiseLinearConstraint( xVarName, yplVarName, numOfPts, xpts, ypts );
    terms.append( GurobiWrapper::Term( 1, yplVarName ) );
    terms.append( GurobiWrapper::Term( -1, yVarName ) );

    // add secant lines
    if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        String binVarName = sigmoid->getBinVarName();
        ASSERT( binVarName != "" );

        if ( FloatUtils::gte( newSplitPoint, 0 ) )
            gurobi.addLeqIndicatorConstraint( binVarName, 1, terms, 0 );
        else
            gurobi.addGeqIndicatorConstraint( binVarName, 0, terms, 0 );
    }
    else
    {
        if ( FloatUtils::gte( newSplitPoint, 0 ) )
            gurobi.addLeqConstraint( terms, 0 );
        else
            gurobi.addGeqConstraint( terms, 0 );
    }
}