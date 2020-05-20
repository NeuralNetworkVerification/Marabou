/*********************                                                        */
/*! \file MaxConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Debug.h"
#include "FloatUtils.h"
#include "IConstraintBoundTightener.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "List.h"
#include "MStringf.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearCaseSplit.h"
#include "MarabouError.h"
#include "Statistics.h"
#include <algorithm>

#ifdef _WIN32
#undef max
#undef min
#endif

MaxConstraint::MaxConstraint( unsigned f, const Set<unsigned> &elements )
    : _f( f )
    , _elements( elements )
    , _maxIndexSet( false )
    , _maxLowerBound( FloatUtils::negativeInfinity() )
    , _obsolete( false )
{
}

MaxConstraint::MaxConstraint( const String &serializedMax )
{
    String constraintType = serializedMax.substring( 0, 3 );
    ASSERT( constraintType == String( "max" ) );

    // remove the constraint type in serialized form
    String serializedValues = serializedMax.substring( 4, serializedMax.length() - 4 );
    List<String> values = serializedValues.tokenize( "," );

    auto valuesIter = values.begin();
    unsigned f = atoi( valuesIter->ascii() );
    ++valuesIter;

    Set<unsigned> elements;
    for ( ; valuesIter != values.end(); ++valuesIter )
        elements.insert( atoi( valuesIter->ascii() ) );

    *(this) = MaxConstraint( f, elements );
}

MaxConstraint::~MaxConstraint()
{
    _elements.clear();
}

PiecewiseLinearFunctionType MaxConstraint::getType() const
{
    return PiecewiseLinearFunctionType::MAX;
}

PiecewiseLinearConstraint *MaxConstraint::duplicateConstraint() const
{
    MaxConstraint *clone = new MaxConstraint( _f, _elements );
    *clone = *this;
    return clone;
}

void MaxConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const MaxConstraint *max = dynamic_cast<const MaxConstraint *>( state );
    *this = *max;
}

void MaxConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _f );
    for ( unsigned element : _elements )
        tableau->registerToWatchVariable( this, element );
}

void MaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _f );
    for ( unsigned element : _elements )
        tableau->unregisterToWatchVariable( this, element );
}

void MaxConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( variable != _f && ( !_maxIndexSet || _assignment.get( _maxIndex ) < value ) )
	  {
        _maxIndex = variable;
        _maxIndexSet = true;
	  }
    _assignment[variable] = value;
}

void MaxConstraint::notifyLowerBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( value, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = value;

    bool maxErased = false;

    if ( _elements.exists( variable ) && FloatUtils::gt( value, _maxLowerBound ) )
    {
        _maxLowerBound = value;
        List<unsigned> toRemove;
        for ( auto element : _elements )
        {
			if ( element == variable )
				continue;
            if ( _upperBounds.exists( element ) &&
                 FloatUtils::lt( _upperBounds[element], value ) )
            {
                toRemove.append( element );
            }
        }
        for ( unsigned removeVar : toRemove )
        {
            _elements.erase( removeVar );
            if ( _maxIndex == removeVar )
                maxErased = true;
        }
    }

	if ( maxErased )
		resetMaxIndex();

    if ( isActive() && _constraintBoundTightener )
    {
        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // can focus only on the newly learned bound and possible consequences.
        List<Tightening> tightenings;
        getEntailedTightenings( tightenings );
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
                _constraintBoundTightener->registerTighterLowerBound( tightening._variable, tightening._value );
            else if ( tightening._type == Tightening::UB )
                _constraintBoundTightener->registerTighterUpperBound( tightening._variable, tightening._value );
        }
    }
}

void MaxConstraint::notifyUpperBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( value, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = value;

    if ( _elements.exists( variable ) && FloatUtils::lt( value, _maxLowerBound ) )
    {
        _elements.erase( variable );
    }

    // There is no need to recompute the max lower bound and max index here.

    if ( isActive() && _constraintBoundTightener )
    {
        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // can focus only on the newly learned bound and possible consequences.
        List<Tightening> tightenings;
        getEntailedTightenings( tightenings );
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
                _constraintBoundTightener->registerTighterLowerBound( tightening._variable, tightening._value );
            else if ( tightening._type == Tightening::UB )
                _constraintBoundTightener->registerTighterUpperBound( tightening._variable, tightening._value );
        }
    }
}

void MaxConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    // Lower and upper bounds for the f variable
    double fLB = _lowerBounds.exists( _f ) ? _lowerBounds.get( _f ) : FloatUtils::negativeInfinity();
    double fUB = _upperBounds.exists( _f ) ? _upperBounds.get( _f ) : FloatUtils::infinity();

    // Compute the maximal bounds (lower and upper) for the elements
    double maxElementLB = FloatUtils::negativeInfinity();
    double maxElementUB = FloatUtils::negativeInfinity();

    for ( const auto &element : _elements )
    {
        if ( _lowerBounds.exists( element ) )
            maxElementLB = FloatUtils::max( _lowerBounds[element], maxElementLB );

        if ( !_upperBounds.exists( element ) )
            maxElementUB = FloatUtils::infinity();
        else
            maxElementUB = FloatUtils::max( _upperBounds[element], maxElementUB );
	}

    // fUB and maxElementUB need to be equal. If not, the lower of the two wins.
    if ( FloatUtils::areDisequal( fUB, maxElementUB ) )
	{
	    if ( FloatUtils::gt( fUB, maxElementUB ) )
		{
		    tightenings.append( Tightening( _f, maxElementUB, Tightening::UB ) );
		}
	    else
		{
		    for ( const auto &element : _elements )
			{
			    if ( !_upperBounds.exists( element ) || FloatUtils::gt( _upperBounds[element], fUB ) )
                    tightenings.append( Tightening( element, fUB, Tightening::UB ) );
			}
		}
	}

    // fLB cannot be smaller than maxElementLB
    if ( FloatUtils::lt( fLB, maxElementLB ) )
        tightenings.append( Tightening( _f, maxElementLB, Tightening::LB ) );
    else if ( _elements.size() == 1 )
    {
        // Special case: there is only one element. In that case, the tighter lower
        // bound (in this case, f's) wins.
        tightenings.append( Tightening( *_elements.begin(), fLB, Tightening::LB ) );
    }

    // TODO: can we derive additional bounds?
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable );
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
    List<unsigned> result;
    for ( auto element : _elements )
        result.append( element );
    result.append( _f );
    return result;
}

bool MaxConstraint::satisfied() const
{
    if ( !( _assignment.exists( _f ) && _assignment.size() > 1 ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double fValue = _assignment.get( _f );
    return FloatUtils::areEqual( _assignment.get( _maxIndex ), fValue );
}

void MaxConstraint::resetMaxIndex()
{
    double maxValue = FloatUtils::negativeInfinity();
    for ( auto element : _elements )
    {
        ASSERT( _assignment.exists( element ) );
        double elementValue = _assignment.get( element );
        if ( elementValue > maxValue )
        {
            maxValue = elementValue;
            _maxIndex = element;
        }
    }
	ASSERT( FloatUtils::isFinite( maxValue ) ); // || _elements.empty() );
    _maxIndexSet = FloatUtils::isFinite( maxValue );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _f ) && _assignment.size() > 1 );

    double fValue = _assignment.get( _f );
    double maxVal = _assignment.get( _maxIndex );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations
    //	1. f is greater than maxVal
    //	2. f is less than maxVal
    //  3. if f is greater than all variables except 1

    if ( FloatUtils::gt( fValue, maxVal ) )
	{
	    fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
	    for ( auto elem : _elements )
		{
		    fixes.append( PiecewiseLinearConstraint::Fix( elem, fValue ) );
		}
	}
    else if ( FloatUtils::lt( fValue, maxVal ) )
	{
	    fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );

        unsigned greaterVar;
        unsigned numGreater = 0;
        for ( auto elem: _elements )
        {
            if ( _assignment.exists( elem ) && FloatUtils::gt( _assignment[elem], fValue ) )
            {
                numGreater++;
                greaterVar = elem;
            }
        }
        if ( numGreater == 1 )
        {
            fixes.append( PiecewiseLinearConstraint::Fix( greaterVar, fValue ) );
        }
	}

    return fixes;
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getSmartFixes( ITableau * ) const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _f ) && _assignment.size() > 1 );

    // TODO
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> MaxConstraint::getCaseSplits() const
{
    if ( phaseFixed() )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    ASSERT(	_assignment.exists( _f ) );

    List<PiecewiseLinearCaseSplit> splits;
    for ( unsigned element : _elements )
	{
        splits.append( getSplit( element ) );
	}
    return splits;
}

bool MaxConstraint::phaseFixed() const
{
    return _elements.size() == 1;
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    ASSERT( phaseFixed() );
    return getSplit( *( _elements.begin() ) );
}

PiecewiseLinearCaseSplit MaxConstraint::getSplit( unsigned argMax ) const
{
    ASSERT( _assignment.exists( argMax ) );
    PiecewiseLinearCaseSplit maxPhase;

    // maxArg - f = 0
    Equation maxEquation( Equation::EQ );
    maxEquation.addAddend( 1, argMax );
    maxEquation.addAddend( -1, _f );
    maxEquation.setScalar( 0 );
    maxPhase.addEquation( maxEquation );

    // store bound tightenings as well
    // go over all other elements;
    // their upper bound cannot exceed upper bound of argmax
    for ( unsigned other : _elements )
	{
	    if ( argMax == other )
            continue;

	    Equation gtEquation( Equation::GE );

	    // argMax >= other
	    gtEquation.addAddend( -1, other );
	    gtEquation.addAddend( 1, argMax );
	    gtEquation.setScalar( 0 );
	    maxPhase.addEquation( gtEquation );

        if ( _upperBounds.exists( argMax ) )
        {
            if ( !_upperBounds.exists( other ) ||
                 FloatUtils::gt( _upperBounds[other], _upperBounds[argMax] ) )
                maxPhase.storeBoundTightening( Tightening( other, _upperBounds[argMax], Tightening::UB ) );
        }
	}

    return maxPhase;
}

void MaxConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    _lowerBounds[newIndex] = _lowerBounds[oldIndex];
    _upperBounds[newIndex] = _upperBounds[oldIndex];
    if ( oldIndex == _f )
        _f = newIndex;
    else
	{
	    _elements.erase( oldIndex );
	    _elements.insert( newIndex );
	}
}

bool MaxConstraint::constraintObsolete() const
{
    return _obsolete;
}

void MaxConstraint::eliminateVariable( unsigned var, double /*value*/ )
{
    _elements.erase( var );
    if ( var == _f || getParticipatingVariables().size() == 1 )
        _obsolete = true;
}

void MaxConstraint::addAuxiliaryEquations( InputQuery &inputQuery )
{
    for ( auto element : _elements )
    {
        // Create an aux variable
        unsigned auxVariable = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( auxVariable + 1 );

        // f >= element, or f - elemenet - aux = 0, for non-negative aux
        Equation equation( Equation::EQ );
        equation.addAddend( 1.0, _f );
        equation.addAddend( -1.0, element );
        equation.addAddend( -1.0, auxVariable );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        // Set the bounds for the aux variable
        inputQuery.setLowerBound( auxVariable, 0 );

        // Todo: upper bound for aux?
    }
}

String MaxConstraint::serializeToString() const
{
    // Output format: max,f,element_1,element_2,element_3,...
    Stringf output = Stringf( "max,%u", _f );
    for ( const auto &element : _elements )
        output += Stringf( ",%u", element );
    return output;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
