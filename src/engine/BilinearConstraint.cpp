/*********************                                                        */
/*! \file BilinearConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in BilinearConstraint.h.
 **/

#include "BilinearConstraint.h"

#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "NonlinearConstraint.h"
#include "Query.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__( x )
#endif

BilinearConstraint::BilinearConstraint( unsigned b1, unsigned b2, unsigned f )
    : NonlinearConstraint()
    , _b1( b1 )
    , _b2( b2 )
    , _f( f )
{
}

BilinearConstraint::BilinearConstraint( const String &serializedBilinear )
{
    String constraintType = serializedBilinear.substring( 0, 8 );
    ASSERT( constraintType == String( "bilinear" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedBilinear.substring( 9, serializedBilinear.length() - 9 );
    List<String> tokens = serializedValues.tokenize( "," );
    Vector<String> tokensVec;
    for ( const auto &token : tokens )
        tokensVec.append( token );

    _b1 = atoi( tokensVec[0].ascii() );
    _b2 = atoi( tokensVec[1].ascii() );
    _f = atoi( tokensVec[2].ascii() );
}

NonlinearFunctionType BilinearConstraint::getType() const
{
    return NonlinearFunctionType::BILINEAR;
}

NonlinearConstraint *BilinearConstraint::duplicateConstraint() const
{
    BilinearConstraint *clone = new BilinearConstraint( _b1, _b2, _f );
    *clone = *this;
    return clone;
}

void BilinearConstraint::restoreState( const NonlinearConstraint *state )
{
    const BilinearConstraint *bilinear = dynamic_cast<const BilinearConstraint *>( state );
    *this = *bilinear;
}

void BilinearConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b1 );
    tableau->registerToWatchVariable( this, _b2 );
    tableau->registerToWatchVariable( this, _f );
}

void BilinearConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b1 );
    tableau->unregisterToWatchVariable( this, _b2 );
    tableau->unregisterToWatchVariable( this, _f );
}

void BilinearConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    tightenLowerBound( variable, bound );
}

void BilinearConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    tightenUpperBound( variable, bound );
}

bool BilinearConstraint::participatingVariable( unsigned variable ) const
{
    return variable == _b1 || variable == _b2 || variable == _f;
}

List<unsigned> BilinearConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b1, _b2, _f } );
}

void BilinearConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b1 || oldIndex == _b2 || oldIndex == _f );

    ASSERT( !_lowerBounds.exists( newIndex ) && !_upperBounds.exists( newIndex ) &&
            newIndex != _b1 && newIndex != _b2 && newIndex != _f );

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

    if ( oldIndex == _b1 )
        _b1 = newIndex;
    if ( oldIndex == _b2 )
        _b2 = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
}

void BilinearConstraint::eliminateVariable( __attribute__( ( unused ) ) unsigned variable,
                                            __attribute__( ( unused ) ) double fixedValue )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                        "Eliminate variable from a Bilinear constraint" );
}

bool BilinearConstraint::constraintObsolete() const
{
    return false;
}

void BilinearConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    if ( existsLowerBound( _b1 ) && FloatUtils::isFinite( getLowerBound( _b1 ) ) &&
         existsLowerBound( _b2 ) && FloatUtils::isFinite( getLowerBound( _b2 ) ) &&
         existsUpperBound( _b1 ) && FloatUtils::isFinite( getUpperBound( _b1 ) ) &&
         existsUpperBound( _b2 ) && FloatUtils::isFinite( getUpperBound( _b2 ) ) )
    {
        double min = FloatUtils::infinity();
        double max = FloatUtils::negativeInfinity();

        double value = getLowerBound( _b1 ) * getLowerBound( _b2 );
        if ( value < min )
            min = value;
        if ( value > max )
            max = value;

        value = getLowerBound( _b1 ) * getUpperBound( _b2 );
        if ( value < min )
            min = value;
        if ( value > max )
            max = value;

        value = getUpperBound( _b1 ) * getLowerBound( _b2 );
        if ( value < min )
            min = value;
        if ( value > max )
            max = value;

        value = getUpperBound( _b1 ) * getUpperBound( _b2 );
        if ( value < min )
            min = value;
        if ( value > max )
            max = value;

        tightenings.append( Tightening( _f, min, Tightening::LB ) );
        tightenings.append( Tightening( _f, max, Tightening::UB ) );
    }
}

bool BilinearConstraint::satisfied() const
{
    if ( !( existsAssignment( _b1 ) && existsAssignment( _b2 ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double b1Value = getAssignment( _b1 );
    double b2Value = getAssignment( _b2 );
    double fValue = getAssignment( _f );

    return FloatUtils::areEqual(
        b1Value * b2Value, fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
}

String BilinearConstraint::serializeToString() const
{
    return Stringf( "bilinear,%u,%u,%u", _b1, _b2, _f );
}

Vector<unsigned> BilinearConstraint::getBs() const
{
    return { _b1, _b2 };
}

unsigned BilinearConstraint::getF() const
{
    return _f;
}
