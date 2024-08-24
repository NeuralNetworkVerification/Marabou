/*********************                                                        */
/*! \file MILPEncoder.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "MILPEncoder.h"

#include "DeepPolySoftmaxElement.h"
#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "TimeUtils.h"

MILPEncoder::MILPEncoder( const ITableau &tableau )
    : _tableau( tableau )
    , _statistics( NULL )
{
}

void MILPEncoder::encodeQuery( GurobiWrapper &gurobi, const Query &inputQuery, bool relax )
{
    struct timespec start = TimeUtils::sampleMicro();

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
        if ( plConstraint->constraintObsolete() )
        {
            continue;
        }
        switch ( plConstraint->getType() )
        {
        case PiecewiseLinearFunctionType::RELU:
            encodeReLUConstraint( gurobi, (ReluConstraint *)plConstraint, relax );
            break;
        case PiecewiseLinearFunctionType::LEAKY_RELU:
            encodeLeakyReLUConstraint( gurobi, (LeakyReluConstraint *)plConstraint, relax );
            break;
        case PiecewiseLinearFunctionType::MAX:
            encodeMaxConstraint( gurobi, (MaxConstraint *)plConstraint, relax );
            break;
        case PiecewiseLinearFunctionType::SIGN:
            encodeSignConstraint( gurobi, (SignConstraint *)plConstraint, relax );
            break;
        case PiecewiseLinearFunctionType::ABSOLUTE_VALUE:
            encodeAbsoluteValueConstraint( gurobi, (AbsoluteValueConstraint *)plConstraint, relax );
            break;
        case PiecewiseLinearFunctionType::DISJUNCTION:
            encodeDisjunctionConstraint( gurobi, (DisjunctionConstraint *)plConstraint, relax );
            break;
        default:
            throw MarabouError( MarabouError::UNSUPPORTED_PIECEWISE_LINEAR_CONSTRAINT,
                                "GurobiWrapper::encodeQuery: "
                                "Unsupported piecewise-linear constraints\n" );
        }
    }

    // Add Nonlinear Constraints
    for ( const auto &nlConstraint : inputQuery.getNonlinearConstraints() )
    {
        switch ( nlConstraint->getType() )
        {
        case NonlinearFunctionType::SIGMOID:
            encodeSigmoidConstraint( gurobi, (SigmoidConstraint *)nlConstraint );
            break;
        case NonlinearFunctionType::SOFTMAX:
            encodeSoftmaxConstraint( gurobi, (SoftmaxConstraint *)nlConstraint );
            break;
        case NonlinearFunctionType::BILINEAR:
            encodeBilinearConstraint( gurobi, (BilinearConstraint *)nlConstraint, relax );
            break;
        case NonlinearFunctionType::ROUND:
            encodeRoundConstraint( gurobi, (RoundConstraint *)nlConstraint, relax );
            break;
        default:
            throw MarabouError( MarabouError::UNSUPPORTED_TRANSCENDENTAL_CONSTRAINT,
                                "GurobiWrapper::encodeQuery: "
                                "Unsupported non-linear constraints\n" );
        }
    }

    gurobi.updateModel();

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,
                                       TimeUtils::timePassed( start, end ) );
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
        terms.append( GurobiWrapper::Term( term._coefficient, Stringf( "x%u", term._variable ) ) );
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

void MILPEncoder::encodeReLUConstraint( GurobiWrapper &gurobi, ReluConstraint *relu, bool relax )
{
    if ( !relu->isActive() || relu->phaseFixed() )
    {
        ASSERT( relu->auxVariableInUse() );
        ASSERT( ( FloatUtils::gte( _tableau.getLowerBound( relu->getB() ), 0 ) &&
                  FloatUtils::lte( _tableau.getLowerBound( relu->getAux() ), 0 ) ) ||
                ( FloatUtils::lte( _tableau.getUpperBound( relu->getB() ), 0 ) &&
                  FloatUtils::lte( _tableau.getUpperBound( relu->getF() ), 0 ) ) );
        return;
    }

    /*
      We have added f - b >= 0 and f >= 0. Additionally, we add
      f - b <= (1 - a) * (- lb_b) and f <= a * ub_f.

      When a = 1, the constraints become:
          f - b <= 0, f <= ub_f.
      When a = 0, the constriants become:
          f - b <= - lb_b, f <= 0
    */
    gurobi.addVariable( Stringf( "a%u", _binVarIndex ),
                        0,
                        1,
                        relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY );

    unsigned sourceVariable = relu->getB();
    unsigned targetVariable = relu->getF();
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double targetUb = _tableau.getUpperBound( targetVariable );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
    terms.append( GurobiWrapper::Term( -sourceLb, Stringf( "a%u", _binVarIndex ) ) );
    gurobi.addLeqConstraint( terms, -sourceLb );

    terms.clear();
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -targetUb, Stringf( "a%u", _binVarIndex++ ) ) );
    gurobi.addLeqConstraint( terms, 0 );
}

void MILPEncoder::encodeLeakyReLUConstraint( GurobiWrapper &gurobi,
                                             LeakyReluConstraint *lRelu,
                                             bool relax )
{
    if ( !lRelu->isActive() || lRelu->phaseFixed() )
    {
        return;
    }
    unsigned sourceVariable = lRelu->getB();
    unsigned targetVariable = lRelu->getF();
    double slope = lRelu->getSlope();
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double sourceUb = _tableau.getUpperBound( sourceVariable );

    if ( sourceLb >= 0 )
    {
        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addEqConstraint( terms, 0 );
    }
    else if ( sourceUb <= 0 )
    {
        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -slope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addEqConstraint( terms, 0 );
    }
    else
    {
        if ( relax )
        {
            /*
              We have added f - b >= 0 and f >= 0. Additionally, we add
                (ub - slope * lb) / (ub - lb) * (b - ub) >= (f - ub)
              which is the upper bound in the triangular relaxation
            */

            double lambda = ( sourceUb - slope * sourceLb ) / ( sourceUb - sourceLb );
            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( lambda, Stringf( "x%u", sourceVariable ) ) );
            terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", targetVariable ) ) );
            gurobi.addGeqConstraint( terms, ( lambda - 1 ) * sourceUb );
        }
        else
        {
            double xPoints[3];
            double yPoints[3];
            xPoints[0] = sourceLb;
            yPoints[0] = slope * sourceLb;
            xPoints[1] = 0;
            yPoints[1] = 0;
            xPoints[2] = sourceUb;
            yPoints[2] = sourceUb;
            gurobi.addPiecewiseLinearConstraint( Stringf( "x%u", sourceVariable ),
                                                 Stringf( "x%u", targetVariable ),
                                                 3,
                                                 xPoints,
                                                 yPoints );
        }
    }
}

void MILPEncoder::encodeMaxConstraint( GurobiWrapper &gurobi, MaxConstraint *max, bool relax )
{
    if ( !max->isActive() )
        return;

    List<GurobiWrapper::Term> terms;
    List<PhaseStatus> phases = max->getAllCases();
    for ( unsigned i = 0; i < phases.size(); ++i )
    {
        // add a binary variable for each disjunct
        gurobi.addVariable( Stringf( "a%u_%u", _binVarIndex, i ),
                            0,
                            1,
                            relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY );

        terms.append( GurobiWrapper::Term( 1, Stringf( "a%u_%u", _binVarIndex, i ) ) );
    }

    // add constraint: a_1 + a_2 + ... + = 1
    gurobi.addEqConstraint( terms, 1 );

    terms.clear();
    unsigned index = 0;
    for ( const auto &phase : phases )
    {
        String binVarName = Stringf( "a%u_%u", _binVarIndex, index );
        PiecewiseLinearCaseSplit split = max->getCaseSplit( phase );
        if ( phase == MAX_PHASE_ELIMINATED )
        {
            /*
              We had y - eliminated value >= 0
              We add y - eliminated-value <= (1 - a) * (ub_y - eliminated-value),
              which becomes y + (ub_y - eliminated-value) * a <= ub_y
            */
            unsigned y = split.getBoundTightenings().begin()->_variable;
            double yUb = _tableau.getUpperBound( y );
            double eliminatedValue = split.getBoundTightenings().begin()->_value;

            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", y ) ) );
            terms.append( GurobiWrapper::Term( yUb - eliminatedValue, binVarName ) );
            gurobi.addLeqConstraint( terms, yUb );
        }
        else
        {
            /*
              We added aux_i >= 0, for each x.
              We now add, aux_i <= (1 - a) * (ub_aux)
            */
            DEBUG( {
                ASSERT( split.getBoundTightenings().size() == 1 );
                ASSERT( split.getEquations().size() == 0 );
            } );
            unsigned aux = split.getBoundTightenings().begin()->_variable;
            double auxUb = _tableau.getUpperBound( aux );
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", aux ) ) );
            terms.append( GurobiWrapper::Term( auxUb, binVarName ) );
            gurobi.addLeqConstraint( terms, auxUb );
        }
        terms.clear();
        ++index;
    }

    _binVarIndex++;
}

void MILPEncoder::encodeAbsoluteValueConstraint( GurobiWrapper &gurobi,
                                                 AbsoluteValueConstraint *abs,
                                                 bool relax )
{
    ASSERT( abs->auxVariablesInUse() );

    if ( !abs->isActive() || abs->phaseFixed() )
    {
        ASSERT( ( FloatUtils::gte( _tableau.getLowerBound( abs->getB() ), 0 ) &&
                  FloatUtils::lte( _tableau.getUpperBound( abs->getPosAux() ), 0 ) ) ||
                ( FloatUtils::lte( _tableau.getUpperBound( abs->getB() ), 0 ) &&
                  FloatUtils::lte( _tableau.getUpperBound( abs->getNegAux() ), 0 ) ) );
        return;
    }

    unsigned sourceVariable = abs->getB();
    unsigned targetVariable = abs->getF();
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double sourceUb = _tableau.getUpperBound( sourceVariable );
    double targetUb = _tableau.getUpperBound( targetVariable );

    ASSERT( FloatUtils::isPositive( sourceUb ) && FloatUtils::isNegative( sourceLb ) );

    /*
      We have added f - b >= 0 and f + b >= 0. We add
      f - b <= (1 - a) * (ub_f - lb_b) and f + b <= a * (ub_f + ub_b)

      When a = 1, the constraints become:
      f - b <= 0, f + b <= ub_f + ub_b.
      When a = 0, the constriants become:
      f - b <= ub_f - lb_b, f + b <= 0
    */
    gurobi.addVariable( Stringf( "a%u", _binVarIndex ),
                        0,
                        1,
                        relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -1, Stringf( "x%u", sourceVariable ) ) );
    terms.append( GurobiWrapper::Term( targetUb - sourceLb, Stringf( "a%u", _binVarIndex ) ) );
    gurobi.addLeqConstraint( terms, targetUb - sourceLb );

    terms.clear();
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", sourceVariable ) ) );
    terms.append( GurobiWrapper::Term( -( targetUb + sourceUb ), Stringf( "a%u", _binVarIndex ) ) );
    gurobi.addLeqConstraint( terms, 0 );
    ++_binVarIndex;
}

void MILPEncoder::encodeDisjunctionConstraint( GurobiWrapper &gurobi,
                                               DisjunctionConstraint *disj,
                                               bool relax )
{
    if ( !disj->isActive() )
        return;

    // terms for Gurobi
    List<GurobiWrapper::Term> terms;
    List<PiecewiseLinearCaseSplit> disjuncts = disj->getCaseSplits();
    for ( unsigned i = 0; i < disjuncts.size(); ++i )
    {
        // add a binary variable for each disjunct
        gurobi.addVariable( Stringf( "a%u_%u", _binVarIndex, i ),
                            0,
                            1,
                            relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY );

        terms.append( GurobiWrapper::Term( 1, Stringf( "a%u_%u", _binVarIndex, i ) ) );
    }

    // add constraint: a_1 + a_2 + ... + >= 1
    gurobi.addGeqConstraint( terms, 1 );

    // Add each disjunct as indicator constraints
    terms.clear();
    unsigned index = 0;
    for ( const auto &disjunct : disjuncts )
    {
        String binVarName = Stringf( "a%u_%u", _binVarIndex, index );
        for ( const auto &tightening : disjunct.getBoundTightenings() )
        {
            // add indicator constraint: a_1 => disjunct1, etc.
            terms.append(
                GurobiWrapper::Term( 1, getVariableNameFromVariable( tightening._variable ) ) );
            if ( tightening._type == Tightening::UB )
                gurobi.addLeqIndicatorConstraint( binVarName, 1, terms, tightening._value );
            else
                gurobi.addGeqIndicatorConstraint( binVarName, 1, terms, tightening._value );
            terms.clear();
        }
        ++index;
    }

    _binVarIndex++;
}

void MILPEncoder::encodeSignConstraint( GurobiWrapper &gurobi, SignConstraint *sign, bool relax )
{
    ASSERT( GlobalConfiguration::PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING );

    if ( !sign->isActive() || sign->phaseFixed() )
    {
        ASSERT( ( FloatUtils::gte( _tableau.getLowerBound( sign->getB() ), 0 ) &&
                  FloatUtils::areEqual( _tableau.getLowerBound( sign->getF() ), 1 ) ) ||
                ( FloatUtils::lte( _tableau.getUpperBound( sign->getB() ), 0 ) &&
                  FloatUtils::areEqual( _tableau.getUpperBound( sign->getF() ), -1 ) ) );
        return;
    }

    unsigned targetVariable = sign->getF();
    DEBUG( {
        unsigned sourceVariable = sign->getB();

        double sourceLb = _tableau.getLowerBound( sourceVariable );
        double sourceUb = _tableau.getUpperBound( sourceVariable );
        ASSERT( !FloatUtils::isNegative( sourceUb ) && FloatUtils::isNegative( sourceLb ) );
    } );

    /*
      We have added f <= -2/lb b + 1 and f >= 2/ub * b - 1. We just need to specify
      f is either -1 or 1. That is f = 2 * (a - 0.5)

      f is 1 if a is 1 and -1 if a is 0.
      Moreover, when f is 1, 1 <= -2 / lb_b * b + 1, thus, b >= 0.
      When f is -1, -1 >= 2/ub_b * b - 1, thus, b <= 0.
    */
    gurobi.addVariable( Stringf( "a%u", _binVarIndex ),
                        0,
                        1,
                        relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY );

    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
    terms.append( GurobiWrapper::Term( -2, Stringf( "a%u", _binVarIndex ) ) );
    gurobi.addEqConstraint( terms, -1 );

    ++_binVarIndex;
}

void MILPEncoder::encodeSigmoidConstraint( GurobiWrapper &gurobi, SigmoidConstraint *sigmoid )
{
    unsigned sourceVariable = sigmoid->getB(); // x_b
    unsigned targetVariable = sigmoid->getF(); // x_f
    double sourceLb = _tableau.getLowerBound( sourceVariable );
    double sourceUb = _tableau.getUpperBound( sourceVariable );

    if ( sourceLb == sourceUb )
    {
        return;
    }
    else if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        List<GurobiWrapper::Term> terms;
        String binVarName = Stringf( "a%u", _binVarIndex ); // a = 1 -> the case where x_b >= 0,
                                                            // otherwise where x_b <= 0
        gurobi.addVariable( binVarName, 0, 1, GurobiWrapper::BINARY );

        // Constraint where x_b >= 0
        // Upper line is tangent and lower line is secant for an overapproximation with a
        // linearization.

        int binVal = 1;

        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        double tangentPoint = sourceUb / 2;
        double yAtTangentPoint = sigmoid->sigmoid( tangentPoint );
        double tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint(
            binVarName, binVal, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
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
        // Upper line is secant and lower line is tangent for an overapproximation with a
        // linearization.

        binVal = 0;

        // tangent line: x_f = tangentSlope * (x_b - tangentPoint) + yAtTangentPoint
        tangentPoint = sourceLb / 2;
        yAtTangentPoint = sigmoid->sigmoid( tangentPoint );
        tangentSlope = sigmoid->sigmoidDerivative( tangentPoint );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -tangentSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addGeqIndicatorConstraint(
            binVarName, binVal, terms, -tangentSlope * tangentPoint + yAtTangentPoint );
        terms.clear();

        // secant line: x_f = secantSlope * (x_b - sourceLb) + y_l
        y_u = y_l;
        y_l = sigmoid->sigmoid( sourceLb );
        secantSlope = ( y_u - y_l ) / ( 0 - sourceLb );
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -secantSlope, Stringf( "x%u", sourceVariable ) ) );
        gurobi.addLeqIndicatorConstraint(
            binVarName, binVal, terms, -secantSlope * sourceLb + y_l );
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

void MILPEncoder::encodeSoftmaxConstraint( GurobiWrapper &gurobi, SoftmaxConstraint *softmax )
{
    Vector<double> sourceLbs;
    Vector<double> sourceUbs;
    Vector<double> sourceMids;
    Vector<double> targetLbs;
    Vector<double> targetUbs;
    Vector<unsigned> sourceVariables = softmax->getInputs();
    Vector<unsigned> targetVariables = softmax->getOutputs();
    unsigned size = sourceVariables.size();
    for ( unsigned i = 0; i < size; ++i )
    {
        double sourceLb = _tableau.getLowerBound( sourceVariables[i] );
        sourceLbs.append( sourceLb - GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
        double sourceUb = _tableau.getUpperBound( sourceVariables[i] );
        sourceUbs.append( sourceUb + GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
        sourceMids.append( ( sourceLb + sourceUb ) / 2 );
        targetLbs.append( _tableau.getLowerBound( targetVariables[i] ) );
        targetUbs.append( _tableau.getUpperBound( targetVariables[i] ) );
    }

    for ( unsigned i = 0; i < size; ++i )
    {
        // The output is fixed, no need to encode symbolic bounds
        if ( FloatUtils::areEqual( targetLbs[i], targetUbs[i] ) )
            continue;
        else
        {
            // lower-bound
            bool wellFormed = true;
            List<GurobiWrapper::Term> terms;
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariables[i] ) ) );
            double symbolicLowerBias;
            bool useLSE2 = false;
            for ( const auto &lb : targetLbs )
            {
                if ( lb > GlobalConfiguration::SOFTMAX_LSE2_THRESHOLD )
                    useLSE2 = true;
            }
            if ( !useLSE2 )
            {
                symbolicLowerBias = NLR::DeepPolySoftmaxElement::LSELowerBound(
                    sourceMids, sourceLbs, sourceUbs, i );
                if ( !FloatUtils::wellFormed( symbolicLowerBias ) )
                    wellFormed = false;
                for ( unsigned j = 0; j < size; ++j )
                {
                    double dldj = NLR::DeepPolySoftmaxElement::dLSELowerBound(
                        sourceMids, sourceLbs, sourceUbs, i, j );
                    if ( !FloatUtils::wellFormed( dldj ) )
                        wellFormed = false;
                    terms.append(
                        GurobiWrapper::Term( -dldj, Stringf( "x%u", sourceVariables[j] ) ) );
                    symbolicLowerBias -= dldj * sourceMids[j];
                }
            }
            else
            {
                symbolicLowerBias = NLR::DeepPolySoftmaxElement::LSELowerBound2(
                    sourceMids, sourceLbs, sourceUbs, i );
                if ( !FloatUtils::wellFormed( symbolicLowerBias ) )
                    wellFormed = false;
                for ( unsigned j = 0; j < size; ++j )
                {
                    double dldj = NLR::DeepPolySoftmaxElement::dLSELowerBound2(
                        sourceMids, sourceLbs, sourceUbs, i, j );
                    if ( !FloatUtils::wellFormed( dldj ) )
                        wellFormed = false;
                    terms.append(
                        GurobiWrapper::Term( -dldj, Stringf( "x%u", sourceVariables[j] ) ) );
                    symbolicLowerBias -= dldj * sourceMids[j];
                }
            }
            if ( wellFormed )
                gurobi.addGeqConstraint( terms, symbolicLowerBias );

            // Upper-bound
            wellFormed = true;
            double symbolicUpperBias =
                NLR::DeepPolySoftmaxElement::LSEUpperBound( sourceMids, targetLbs, targetUbs, i );
            if ( !FloatUtils::wellFormed( symbolicUpperBias ) )
                wellFormed = false;
            terms.clear();
            terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariables[i] ) ) );
            for ( unsigned j = 0; j < size; ++j )
            {
                double dudj = NLR::DeepPolySoftmaxElement::dLSEUpperbound(
                    sourceMids, targetLbs, targetUbs, i, j );
                if ( !FloatUtils::wellFormed( dudj ) )
                    wellFormed = false;
                terms.append( GurobiWrapper::Term( -dudj, Stringf( "x%u", sourceVariables[j] ) ) );
                symbolicUpperBias -= dudj * sourceMids[j];
            }
            if ( wellFormed )
                gurobi.addLeqConstraint( terms, symbolicUpperBias );
        }
    }
}

void MILPEncoder::encodeBilinearConstraint( GurobiWrapper &gurobi,
                                            BilinearConstraint *bilinear,
                                            bool relax )
{
    if ( relax )
    {
        // Encode the DeepPoly abstraction
        auto sourceVariables = bilinear->getBs();
        unsigned sourceVariable1 = sourceVariables[0];
        unsigned sourceVariable2 = sourceVariables[1];
        unsigned targetVariable = bilinear->getF();
        double sourceLb1 = _tableau.getLowerBound( sourceVariable1 );
        double sourceLb2 = _tableau.getLowerBound( sourceVariable2 );
        double sourceUb2 = _tableau.getUpperBound( sourceVariable2 );

        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -sourceLb2, Stringf( "x%u", sourceVariable1 ) ) );
        terms.append( GurobiWrapper::Term( -sourceLb1, Stringf( "x%u", sourceVariable2 ) ) );
        gurobi.addGeqConstraint( terms, -sourceLb1 * sourceLb2 );

        terms.clear();
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -sourceUb2, Stringf( "x%u", sourceVariable1 ) ) );
        terms.append( GurobiWrapper::Term( -sourceLb1, Stringf( "x%u", sourceVariable2 ) ) );
        gurobi.addLeqConstraint( terms, -sourceLb1 * sourceUb2 );
    }
    else
    {
        gurobi.nonConvex();
        auto bs = bilinear->getBs();
        ASSERT( bs.size() == 2 );
        auto f = bilinear->getF();
        gurobi.addBilinearConstraint(
            Stringf( "x%u", bs[0] ), Stringf( "x%u", bs[1] ), Stringf( "x%u", f ) );
        return;
    }
}

void MILPEncoder::encodeRoundConstraint( GurobiWrapper &gurobi, RoundConstraint *round, bool relax )
{
    /*
      We have already introduced during preprocessing
      f - b <= 0.5
      b - f <= 0.5

      Therefore, nothing needs to be done if we are encoding the relaxation.
      Otherwise, we introduce a new integer variable i and assert that f is
      equal to i.
    */
    if ( !relax )
    {
        unsigned targetVariable = round->getF();
        String varName = Stringf( "i%u", _intVarIndex );
        gurobi.addVariable( varName,
                            _tableau.getLowerBound( targetVariable ),
                            _tableau.getUpperBound( targetVariable ),
                            GurobiWrapper::INTEGER );
        List<GurobiWrapper::Term> terms;
        terms.append( GurobiWrapper::Term( 1, Stringf( "x%u", targetVariable ) ) );
        terms.append( GurobiWrapper::Term( -1, Stringf( "i%u", _intVarIndex ) ) );
        gurobi.addEqConstraint( terms, 0 );
        ++_intVarIndex;
    }
}

void MILPEncoder::encodeCostFunction( GurobiWrapper &gurobi, const LinearExpression &cost )
{
    List<GurobiWrapper::Term> terms;
    for ( const auto &pair : cost._addends )
    {
        terms.append( GurobiWrapper::Term( pair.second, Stringf( "x%u", pair.first ) ) );
    }
    gurobi.setCost( terms, cost._constant );
}
