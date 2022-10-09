/*********************                                                        */
/*! \file SoftmaxConstraint.cpp
** \verbatim
** Top contributors (to current version):
**   Haoze (Andrew) Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** See the description of the class in SoftmaxConstraint.h.
**/

#include "SoftmaxConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "TableauRow.h"
#include "TranscendentalConstraint.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

SoftmaxConstraint::SoftmaxConstraint( const Vector<unsigned> &inputs,
                                      const Vector<unsigned> &outputs )
  : TranscendentalConstraint()
  , _inputs( inputs )
  , _outputs( outputs )
{
  ASSERT( _inputs.size() == _outputs.size() );
}

SoftmaxConstraint::SoftmaxConstraint( const String &serializedSoftmax )
{
  String constraintType = serializedSoftmax.substring( 0, 7 );
  ASSERT( constraintType == String( "softmax" ) );

  // Remove the constraint type in serialized form
  String serializedValues = serializedSoftmax.substring( 8, serializedSoftmax.length() - 8 );
  List<String> tokens = serializedValues.tokenize(",");
  unsigned d = atoi(tokens.begin()->ascii());
  unsigned i = 0;
  for ( const auto &token : tokens )
  {
    ++i;
    if ( i == 1 )
      continue;
    if ( i <= d + 1 )
      _inputs.append(atoi(token.ascii()));
    else
      _outputs.append(atoi(token.ascii()));
  }

  ASSERT( _inputs.size() == _outputs.size() );
  ASSERT( _inputs.size() == d );
}

TranscendentalFunctionType SoftmaxConstraint::getType() const
{
  return TranscendentalFunctionType::SOFTMAX;
}

TranscendentalConstraint *SoftmaxConstraint::duplicateConstraint() const
{
  SoftmaxConstraint *clone = new SoftmaxConstraint( _inputs, _outputs );
  *clone = *this;
  return clone;
}

void SoftmaxConstraint::restoreState( const TranscendentalConstraint *state )
{
      const SoftmaxConstraint *softmax =
        dynamic_cast<const SoftmaxConstraint *>( state );
      *this = *softmax;
}

void SoftmaxConstraint::registerAsWatcher( ITableau *tableau )
{
  for ( const auto &input : _inputs )
    tableau->registerToWatchVariable( this, input );
  for ( const auto &output : _outputs )
    tableau->registerToWatchVariable( this, output );
}

void SoftmaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
  for ( const auto &input : _inputs )
    tableau->unregisterToWatchVariable( this, input );
  for ( const auto &output : _outputs )
    tableau->unregisterToWatchVariable( this, output );
}

void SoftmaxConstraint::notifyLowerBound( unsigned variable, double bound )
{
  ASSERT( participatingVariable( variable ) );

  tightenLowerBound(variable, bound);
}

void SoftmaxConstraint::notifyUpperBound( unsigned variable, double bound )
{
  ASSERT( participatingVariable( variable ) );

  tightenUpperBound(variable, bound);
}

bool SoftmaxConstraint::participatingVariable( unsigned variable ) const
{
  return ( _inputs.exists( variable ) || _outputs.exists( variable ) );
}

List<unsigned> SoftmaxConstraint::getParticipatingVariables() const
{
  List<unsigned> toReturn;
  for ( const auto &var : _inputs )
    toReturn.append( var );
  for ( const auto &var : _outputs )
    toReturn.append( var );
  return toReturn;
}

void SoftmaxConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
  ASSERT( participatingVariable( oldIndex ) );
  ASSERT( !participatingVariable( newIndex ) );

  if ( _assignment.exists( oldIndex ) )
    {
      _assignment[newIndex] = _assignment.get( oldIndex );
      _assignment.erase( oldIndex );
    }

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

  for ( unsigned i = 0; i < _inputs.size(); ++i )
    {
      if ( _inputs[i] == oldIndex )
        _inputs[i] = newIndex;
      if ( _outputs[i] == oldIndex )
        _outputs[i] = newIndex;
    }
}

void SoftmaxConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                           __attribute__((unused)) double fixedValue )
{
  throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                      "Eliminate variable from a Softmax constraint" );
}

bool SoftmaxConstraint::constraintObsolete() const
{
  return _inputs.size() == 0;
}

void SoftmaxConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
  for ( const unsigned &var : _outputs )
    tightenings.append(Tightening(var, 0, Tightening::LB));

  for ( const unsigned &var : _outputs )
    tightenings.append(Tightening(var, 1, Tightening::UB));

  Vector<double> lbs;
  Vector<double> ubs;
  for ( const unsigned &var : _inputs ) {
    if ( _lowerBounds.exists(var) && FloatUtils::isFinite(_lowerBounds[var]) )
    {
      tightenings.append(Tightening(var, _lowerBounds[var], Tightening::LB));
      lbs.append(_lowerBounds[var]);
    }
    if ( _upperBounds.exists(var) && FloatUtils::isFinite(_upperBounds[var]) )
    {
      tightenings.append(Tightening(var, _upperBounds[var], Tightening::UB));
      ubs.append(_upperBounds[var]);
    }
  }

  if (lbs.size() == _inputs.size() && ubs.size() == _inputs.size()) {
    for ( unsigned i = 0; i < _outputs.size(); ++i ) {
      unsigned yi = _outputs[i];

      Vector<double> lTilda;
      Vector<double> uTilda;

      xTilda(lbs, ubs[i], lTilda);
      xTilda(ubs, lbs[i], uTilda);

      lTilda[i] = 0;
      uTilda[i] = 0;

      tightenings.append(Tightening(yi, 1 / SE(uTilda), Tightening::LB));
      tightenings.append(Tightening(yi, 1 / SE(lTilda), Tightening::UB));
    }
  }
}

String SoftmaxConstraint::serializeToString() const
{
  String s = Stringf( "softmax" );
  s += Stringf( ",%u", _inputs.size());
  for (const auto &input : _inputs)
    s += Stringf( ",%u", input);
  for (const auto &output : _outputs)
    s += Stringf( ",%u", output);
  return s;
}

const Vector<unsigned> &SoftmaxConstraint::getInputs() const
{
  return _inputs;
}

const Vector<unsigned> &SoftmaxConstraint::getOutputs() const
{
  return _outputs;
}

void SoftmaxConstraint::softmax( const Vector<double> &input, Vector<double> &output )
{
  // A stable softmax implementation: https://stackoverflow.com/questions/42599498/numercially-stable-softmax
  unsigned maxIndex = 0;
  for ( unsigned i = 0; i < input.size(); ++i )
    if (input[i] > input[maxIndex])
      maxIndex = i;

  Vector<double> inputTilda;
  xTilda(input, input[maxIndex], inputTilda);

  output.clear();

  for ( const auto &value : inputTilda )
    output.append(std::exp( value ));

  double se = SE(inputTilda);

  for ( unsigned i = 0; i < output.size(); ++i )
    output[i] /= se;
}

void SoftmaxConstraint::xTilda( const Vector<double> &input,
                                  double value,
                                  Vector<double> &output )
{
  output.clear();
  for ( const auto &i : input )
    output.append(i - value);
}

double SoftmaxConstraint::SE( const Vector<double> &input )
{
  double sum = 0;
  for ( const auto &value : input )
    sum += std::exp( value );
  return sum;
}

double SoftmaxConstraint::LSE( const Vector<double> &input )
{
  return std::log(SE(input));
}
