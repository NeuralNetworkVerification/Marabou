/*********************                                                        */
/*! \file QuadraticConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in QuadraticConstraint.h.
 **/

#include "QuadraticConstraint.h"

#include "TranscendentalConstraint.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

QuadraticConstraint::QuadraticConstraint( unsigned b1, unsigned b2, unsigned f )
    : TranscendentalConstraint()
    , _b1( b1 )
    , _b2( b2 )
    , _f( f )
{
}

QuadraticConstraint::QuadraticConstraint( const String &serializedQuadratic )
{
  String constraintType = serializedQuadratic.substring( 0, 4 );
  ASSERT( constraintType == String( "quad" ) );

  // Remove the constraint type in serialized form
  String serializedValues = serializedQuadratic.substring( 5, serializedQuadratic.length() - 5 );
  List<String> tokens = serializedValues.tokenize(",");
  Vector<String> tokensVec;
  for (const auto &token : tokens)
    tokensVec.append(token);

  _b1 = atoi(tokensVec[0].ascii());
  _b2 = atoi(tokensVec[1].ascii());
  _f = atoi(tokensVec[2].ascii());
}

TranscendentalFunctionType QuadraticConstraint::getType() const
{
    return TranscendentalFunctionType::QUADRATIC;
}

TranscendentalConstraint *QuadraticConstraint::duplicateConstraint() const
{
    QuadraticConstraint *clone = new QuadraticConstraint( _b1, _b2, _f );
    *clone = *this;
    return clone;
}

void QuadraticConstraint::restoreState( const TranscendentalConstraint *state )
{
    const QuadraticConstraint *quad = dynamic_cast<const QuadraticConstraint *>( state );
    *this = *quad;
}

void QuadraticConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b1 );
    tableau->registerToWatchVariable( this, _b2 );
    tableau->registerToWatchVariable( this, _f );
}

void QuadraticConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b1 );
    tableau->unregisterToWatchVariable( this, _b2 );
    tableau->unregisterToWatchVariable( this, _f );
}

void QuadraticConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    tightenLowerBound(variable, bound);
}

void QuadraticConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    tightenUpperBound(variable, bound);
}

bool QuadraticConstraint::participatingVariable( unsigned variable ) const
{
  return variable == _b1 || variable == _b2 || variable == _f;
}

List<unsigned> QuadraticConstraint::getParticipatingVariables() const
{
  return List<unsigned>( { _b1, _b2, _f } );
}

void QuadraticConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b1 || oldIndex == _b2 || oldIndex == _f );

    ASSERT( !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
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

void QuadraticConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
  throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                      "Eliminate variable from a Quadratic constraint" );
}

bool QuadraticConstraint::constraintObsolete() const
{
    return false;
}

void QuadraticConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
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

String QuadraticConstraint::serializeToString() const
{
  return Stringf( "quad,%u,%u,%u", _b1, _b2, _f );
}

List<unsigned> QuadraticConstraint::getBs() const
{
  return {_b1, _b2};
}

unsigned QuadraticConstraint::getF() const
{
    return _f;
}
