/*********************                                                        */
/*! \file SigmoidConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori, Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in SigmoidConstraint.h.
 **/

#include "SigmoidConstraint.h"

#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "LeakyReluConstraint.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "NonlinearConstraint.h"
#include "Query.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__( x )
#endif

SigmoidConstraint::SigmoidConstraint( unsigned b, unsigned f )
    : NonlinearConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

SigmoidConstraint::SigmoidConstraint( const String &serializedSigmoid )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedSigmoid.substring( 0, 7 );
    ASSERT( constraintType == String( "sigmoid" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedSigmoid.substring( 8, serializedSigmoid.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 2 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
}

NonlinearFunctionType SigmoidConstraint::getType() const
{
    return NonlinearFunctionType::SIGMOID;
}

NonlinearConstraint *SigmoidConstraint::duplicateConstraint() const
{
    SigmoidConstraint *clone = new SigmoidConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void SigmoidConstraint::restoreState( const NonlinearConstraint *state )
{
    const SigmoidConstraint *sigmoid = dynamic_cast<const SigmoidConstraint *>( state );
    *this = *sigmoid;
}

void SigmoidConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void SigmoidConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void SigmoidConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( !_boundManager && tightenLowerBound( variable, bound ) )
    {
        if ( variable == _f && !FloatUtils::areEqual( bound, 0 ) &&
             !FloatUtils::areEqual( bound, 1 ) )
            tightenLowerBound( _b, sigmoidInverse( bound ) );
        else if ( variable == _b )
            tightenLowerBound( _f, sigmoid( bound ) );
    }
}

void SigmoidConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( !_boundManager && tightenUpperBound( variable, bound ) )
    {
        if ( variable == _f && !FloatUtils::areEqual( bound, 0 ) &&
             !FloatUtils::areEqual( bound, 1 ) )
            tightenUpperBound( _b, sigmoidInverse( bound ) );
        else if ( variable == _b )
            tightenUpperBound( _f, sigmoid( bound ) );
    }
}

bool SigmoidConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> SigmoidConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void SigmoidConstraint::dump( String &output ) const
{
    output = Stringf( "SigmoidConstraint: x%u = Sigmoid( x%u ).\n", _f, _b );

    output +=
        Stringf( "b in [%s, %s], ",
                 existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                 existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output +=
        Stringf( "f in [%s, %s]",
                 existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                 existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void SigmoidConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_lowerBounds.exists( newIndex ) && !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );

    if ( _lowerBounds.exists( oldIndex ) )
    {
        _lowerBounds[newIndex] = _lowerBounds.get( oldIndex );
        _lowerBounds.erase( oldIndex );
    }

    if ( _upperBounds.exists( oldIndex ) )
    {
        _upperBounds[newIndex] = _upperBounds.get( oldIndex );
        _upperBounds.erase( oldIndex );
    }

    if ( oldIndex == _b )
        _b = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
}

void SigmoidConstraint::eliminateVariable( __attribute__( ( unused ) ) unsigned variable,
                                           __attribute__( ( unused ) ) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Sigmoid constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool SigmoidConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void SigmoidConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) && existsUpperBound( _b ) &&
            existsUpperBound( _f ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );
    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    tightenings.append( Tightening( _b, bLowerBound, Tightening::LB ) );
    tightenings.append( Tightening( _f, fLowerBound, Tightening::LB ) );

    tightenings.append( Tightening( _b, bUpperBound, Tightening::UB ) );
    tightenings.append( Tightening( _f, fUpperBound, Tightening::UB ) );
}

bool SigmoidConstraint::satisfied() const
{
    if ( !( existsAssignment( _b ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    return FloatUtils::areEqual(
        sigmoid( bValue ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
}

String SigmoidConstraint::serializeToString() const
{
    return Stringf( "sigmoid,%u,%u", _f, _b );
}

unsigned SigmoidConstraint::getB() const
{
    return _b;
}

unsigned SigmoidConstraint::getF() const
{
    return _f;
}

double SigmoidConstraint::sigmoid( double x )
{
    x = std::max( -GlobalConfiguration::SIGMOID_CUTOFF_CONSTANT, x );
    x = std::min( GlobalConfiguration::SIGMOID_CUTOFF_CONSTANT, x );
    return 1 / ( 1 + std::exp( -x ) );
}

double SigmoidConstraint::sigmoidInverse( double y )
{
    if ( FloatUtils::areEqual( y, 0 ) )
        return FloatUtils::negativeInfinity();
    else if ( FloatUtils::areEqual( y, 1 ) )
        return FloatUtils::infinity();
    else
        return log( y / ( 1 - y ) );
}

double SigmoidConstraint::sigmoidDerivative( double x )
{
    return sigmoid( x ) * ( 1 - sigmoid( x ) );
}

bool SigmoidConstraint::attemptToRefine( Query &inputQuery ) const
{
    double bValue = inputQuery.getSolutionValue( _b );
    double fValue = inputQuery.getSolutionValue( _f );
    if ( FloatUtils::areEqual(
             sigmoid( bValue ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) )
    {
        // Already satisfied
        return false;
    }
    else if ( FloatUtils::gte( bValue, GlobalConfiguration::SIGMOID_CUTOFF_CONSTANT ) ||
              FloatUtils::lte( bValue, -GlobalConfiguration::SIGMOID_CUTOFF_CONSTANT ) )
    {
        return false;
    }
    else
    {
        /*
          Use the strategy described in "Toward Certified Robustness Against Real-World
          Distribution Shifts" to refine the Sigmoid constraint.

          We need to add a two-phased piecewise-linear constraints
          of the form:

          (aux = beta * _b + sigmoid(bValue) - beta * bValue) /\ x <= bValue) \/
          (aux = gamma * _b + sigmoid(bValue) - gamma * bValue) /\ x >= bValue)

          which can be encoded as

              aux = gamma * leakyrelu(x - bValue, beta/gamma) + sigmoid(bValue)

          if beta/gamma < 1,

          or as

              aux = -beta * leakyrelu(bValue - x, gamma/beta)  + sigmoid(bValue)

          otherwise.

          The value of beta and gamma depends on the values of bValue and fValue

          If fValue > sigmoid(bValue), we add _f <= aux, otherwise, we add _f >= aux
        */

        // Due to the DeepPoly abstraction, assignments at the end points are precise.
        double lb = inputQuery.getLowerBound( _b );
        double ub = inputQuery.getUpperBound( _b );
        double correctfValue = sigmoid( bValue );

        double beta = NAN;
        double gamma = NAN;
        if ( FloatUtils::lt( fValue, correctfValue ) )
        {
            // fValue is below the Sigmoid
            if ( FloatUtils::lt( lb, INFLECTION_POINT ) && FloatUtils::gt( ub, INFLECTION_POINT ) )
            {
                if ( FloatUtils::lt( bValue, 0 ) )
                {
                    // Case 1
                    gamma = std::min( sigmoidDerivative( lb ), sigmoidDerivative( ub ) );
                    if ( FloatUtils::areEqual( bValue, lb ) )
                        beta = gamma;
                    else
                        beta = sigmoidDerivative( bValue );
                }
                else if ( FloatUtils::isZero( bValue ) )
                {
                    // Case 2
                    beta = sigmoidDerivative( bValue );
                    gamma = ( correctfValue - sigmoid( ub ) ) / ( bValue - ub );
                }
                else
                {
                    // Case 3
                    beta = ( sigmoid( lb ) + sigmoidDerivative( lb ) * ( INFLECTION_POINT - lb ) -
                             correctfValue ) /
                           ( INFLECTION_POINT - bValue );
                    if ( FloatUtils::areEqual( bValue, ub ) )
                        gamma = beta;
                    else
                        gamma = ( correctfValue - sigmoid( ub ) ) / ( bValue - ub );
                }
            }
            else
            {
                if ( FloatUtils::gt( bValue, 0 ) )
                {
                    // Case 4
                    beta = ( correctfValue - sigmoid( lb ) ) / ( bValue - lb );
                    if ( FloatUtils::areEqual( bValue, ub ) )
                        gamma = beta;
                    else
                        gamma = ( correctfValue - sigmoid( ub ) ) / ( bValue - ub );
                }
                else
                {
                    // Case 5
                    beta = sigmoidDerivative( bValue );
                    gamma = beta;
                }
            }
        }
        else
        {
            // fValue is above the Sigmoid
            if ( FloatUtils::lt( lb, INFLECTION_POINT ) && FloatUtils::gt( ub, INFLECTION_POINT ) )
            {
                if ( FloatUtils::lt( bValue, 0 ) )
                {
                    // Case 6
                    gamma = ( sigmoid( ub ) - sigmoidDerivative( ub ) * ( ub - INFLECTION_POINT ) -
                              correctfValue ) /
                            ( INFLECTION_POINT - bValue );
                    if ( FloatUtils::areEqual( bValue, lb ) )
                        beta = gamma;
                    else
                        beta = ( correctfValue - sigmoid( lb ) ) / ( bValue - lb );
                }
                else if ( FloatUtils::isZero( bValue ) )
                {
                    // Case 7
                    beta = ( correctfValue - sigmoid( lb ) ) / ( bValue - lb );
                    gamma = sigmoidDerivative( bValue );
                }
                else
                {
                    // Case 8
                    beta = std::min( sigmoidDerivative( lb ), sigmoidDerivative( ub ) );
                    if ( FloatUtils::areEqual( bValue, ub ) )
                        gamma = beta;
                    else
                        gamma = sigmoidDerivative( bValue );
                }
            }
            else
            {
                if ( FloatUtils::gt( bValue, 0 ) )
                {
                    // Case 9
                    beta = sigmoidDerivative( bValue );
                    gamma = beta;
                }
                else
                {
                    // Case 10
                    gamma = ( correctfValue - sigmoid( ub ) ) / ( bValue - ub );
                    if ( FloatUtils::areEqual( bValue, lb ) )
                        beta = gamma;
                    else
                        beta = ( correctfValue - sigmoid( lb ) ) / ( bValue - lb );
                }
            }
        }

        ASSERT( !FloatUtils::isNan( beta ) && beta > 0 && !FloatUtils::isNan( gamma ) &&
                gamma > 0 );

        if ( FloatUtils::lt( beta, gamma ) )
        {
            /*
              Need to encode aux = gamma * leakyrelu(b - bValue, beta/gamma) + sigmoid(bValue)
              which becomes

              aux1 = b - bValue
              aux2 = leakyReLU(aux1, beta/gamma)
              aux3 = gamma * aux2 + sigmoid(bValue)
            */

            unsigned aux1 = inputQuery.getNewVariable();
            unsigned aux2 = inputQuery.getNewVariable();

            {
                Equation e;
                e.addAddend( 1, _b );
                e.addAddend( -1, aux1 );
                e.setScalar( bValue );
                inputQuery.addEquation( e );
            }

            LeakyReluConstraint *r = new LeakyReluConstraint( aux1, aux2, beta / gamma );
            inputQuery.addPiecewiseLinearConstraint( r );

            {
                // if fValue is above sigmoid, we add
                // f <= aux3. Otherwise, we add f >= aux3
                Equation e;
                if ( FloatUtils::gt( fValue, correctfValue ) )
                    e.setType( Equation::LE );
                else
                    e.setType( Equation::GE );
                e.addAddend( 1, _f );
                e.addAddend( -gamma, aux2 );
                e.setScalar( correctfValue );
                inputQuery.addEquation( e );
            }
        }
        else if ( FloatUtils::areEqual( beta, gamma ) )
        {
            /*
              Need to encode aux = beta * _b + sigmoid(bValue) - beta * bValue

              and y <= aux if fValue > corectFValue
                  y >= aux if fValue < corectFValue

              So we don't really need to introduce any aux variables
            */
            Equation e;
            if ( FloatUtils::gt( fValue, correctfValue ) )
                e.setType( Equation::LE );
            else
                e.setType( Equation::GE );
            e.addAddend( 1, _f );
            e.addAddend( -beta, _b );
            e.setScalar( correctfValue - beta * bValue );
            inputQuery.addEquation( e );
        }
        else
        {
            /*
              Need to encode aux = -beta * leakyrelu(bValue - b, gamma/beta)  + sigmoid(bValue)
              which becomes

              aux1 = bValue - b
              aux2 = leakyReLU(aux1, gamma/beta)
              aux3 = -beta * aux2 + sigmoid(bValue)
            */

            unsigned aux1 = inputQuery.getNewVariable();
            unsigned aux2 = inputQuery.getNewVariable();

            {
                Equation e;
                e.addAddend( 1, _b );
                e.addAddend( 1, aux1 );
                e.setScalar( bValue );
                inputQuery.addEquation( e );
            }

            LeakyReluConstraint *r = new LeakyReluConstraint( aux1, aux2, gamma / beta );
            inputQuery.addPiecewiseLinearConstraint( r );

            {
                // if fValue is above sigmoid, we add
                // f <= aux3. Otherwise, we add f >= aux3
                Equation e;
                if ( FloatUtils::gt( fValue, correctfValue ) )
                    e.setType( Equation::LE );
                else
                    e.setType( Equation::GE );
                e.addAddend( 1, _f );
                e.addAddend( beta, aux2 );
                e.setScalar( correctfValue );
                inputQuery.addEquation( e );
            }
        }
        return true;
    }
}
