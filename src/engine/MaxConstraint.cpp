/*********************                                                        */
/*! \file MaxConstraint.cpp
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "Debug.h"
#include "FloatUtils.h"
#include "FreshVariables.h"
#include "ITableau.h"
#include "List.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"
#include "Statistics.h"
#include <algorithm>

MaxConstraint::MaxConstraint( unsigned f, const Set<unsigned> &elements )
    : _f( f )
    , _elements( elements )
    , _phaseFixed( false )
    , _maxLowerBound(FloatUtils::negativeInfinity())
    , _removePL(false)
{
}

MaxConstraint::~MaxConstraint()
{
    _elements.clear();
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
    if ( variable != _f )
	{
	    // Two conditions for _maxIndex to not exist: either _assignment.size()
	    // equals to 0, or the only element in _assignment is _f.
	    // Otherwise, we only replace _maxIndex if the value of _maxIndex is less
	    // than the new value.
	    if ( _assignment.size() == 0 ||
             ( _assignment.exists( _f ) && _assignment.size() == 1 ) ||
             _assignment.get( _maxIndex ) < value )
            _maxIndex = variable;
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

    if ( FloatUtils::gt( value, _maxLowerBound ) )
    {
        _maxLowerBound = value;
        List<unsigned> toRemove;
        for ( auto element : _elements )
        {
            if( _upperBounds.exists( element ) &&
                FloatUtils::lt(_upperBounds[element], value) )
                toRemove.append(element);
        }
        for ( unsigned removeVar: toRemove )
        {
            if ( _lowerBounds.exists( removeVar ) )
                _lowerBounds.erase( removeVar );
            if ( _upperBounds.exists( removeVar ) )
                _upperBounds.erase( removeVar );
            _elements.erase( removeVar );
            _eliminated.insert( removeVar );
        }
    }

    checkForFixedPhaseOnAlterationToBounds();
}

void MaxConstraint::notifyUpperBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( value, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = value;

    if ( FloatUtils::lt( value, _maxLowerBound ) )
    {
        if ( _lowerBounds.exists( variable ) )
            _lowerBounds.erase( variable );
        if (_upperBounds.exists( variable ) )
            _upperBounds.erase( variable );
        _elements.erase( variable );
        _eliminated.insert( variable );
    }

    checkForFixedPhaseOnAlterationToBounds();
}

void MaxConstraint::checkForFixedPhaseOnAlterationToBounds()
{
    // Compute the maximum lowest bound among all elements.
    double maxLowerBound = FloatUtils::negativeInfinity();
    unsigned maxLowerBoundElement = 0;
    for ( const unsigned element : _elements )
    {
        // At the time this API is called, the lower bound is guaranteed to exist
        // for all elements.
        if ( _lowerBounds.exists( element ) &&
             FloatUtils::lt( maxLowerBound, _lowerBounds[element] ) )
        {
            maxLowerBound = _lowerBounds[element];
            maxLowerBoundElement = element;
        }
    }

    // Check to see if it is greater than the upper bound of all other elements.
    _phaseFixed = true;
    for ( const unsigned element : _elements )
    {
        if ( _upperBounds.exists( element ) &&
             element != maxLowerBoundElement &&
             FloatUtils::lt( maxLowerBound, _upperBounds[element] ) )
        {
            _phaseFixed = false;
            break;
        }
    }
    if( _phaseFixed )
        _fixedPhase = maxLowerBoundElement;
}

void MaxConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    // Lower and upper bounds for the f variable
    double fLB = _lowerBounds.get( _f );
    double fUB = _upperBounds.get( _f );

    // Compute the maximal bounds (lower and upper) for the elements
    double maxElementLB = FloatUtils::negativeInfinity();
    double maxElementUB = FloatUtils::negativeInfinity();

    for ( const auto &element : _elements )
	{
	    if ( !_lowerBounds.exists( element ) )
            maxElementLB = FloatUtils::infinity();
	    else
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

    // TODO: can we derive additional bounds?
}

// void MaxConstraint::updateBounds()
// {
// }

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable );
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
    List<unsigned> temp;
    for ( auto element : _elements )
        temp.append( element );
    temp.append( _f );
    return temp;
}

bool MaxConstraint::satisfied() const
{
    if ( !( _assignment.exists( _f ) && _assignment.size() > 1 ) )
        throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

    double fValue = _assignment.get( _f );
    return FloatUtils::areEqual( _assignment.get( _maxIndex ), fValue );
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

	    /*for ( auto elem : _elements )
	      {
	      if ( _assignment.exists( elem ) && FloatUtils::lt( fValue, _assignment.get( elem ) ) )
	      fixes.append( PiecewiseLinearConstraint::Fix( elem, fValue ) );
	      }*/
	}
    return fixes;
}

List<PiecewiseLinearCaseSplit> MaxConstraint::getCaseSplits() const
{
    ASSERT(	_assignment.exists( _f ) );

    List<PiecewiseLinearCaseSplit> splits;
    for ( unsigned element : _elements )
	{
	    if ( _assignment.exists( element ) )
            splits.append( getSplit( element ) );
	}
    return splits;
}

bool MaxConstraint::phaseFixed() const
{
    return _phaseFixed;
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseFixed );
    return getSplit( _fixedPhase );
}

PiecewiseLinearCaseSplit MaxConstraint::getSplit( unsigned argMax ) const
{
    ASSERT( _assignment.exists( argMax ) );
    PiecewiseLinearCaseSplit maxPhase;

    // maxArg - f = 0
    Equation maxEquation;
    maxEquation.addAddend( 1, argMax );
    maxEquation.addAddend( -1, _f );
    maxEquation.setScalar( 0 );
    maxPhase.addEquation( maxEquation, PiecewiseLinearCaseSplit::EQ );

    // store bound tightenings as well
    // go over all other elements;
    // their upper bound cannot exceed upper bound of argmax
    for ( unsigned other : _elements )
	{
	    if ( argMax == other )
            continue;

	    Equation gtEquation;

	    // argMax >= other
	    gtEquation.addAddend( -1, other );
	    gtEquation.addAddend( 1, argMax );
	    gtEquation.setScalar( 0 );
	    maxPhase.addEquation( gtEquation, PiecewiseLinearCaseSplit::GE );

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
    return _removePL;
}

void MaxConstraint::eliminateVariable( unsigned var, double value )
{
    /*if ( _eliminated.size() == 0 || ( _eliminated.size() > 0 && FloatUtils::gt( value, _lowerBounds[_maxElim] ) ) )
      {
      _maxElim = var;
      _lowerBounds[_f] = FloatUtils::max( value, _lowerBounds[_f] );
      }*/

    //_lowerBounds[var] = value;
    //_upperBounds[var] = value;
    if(_lowerBounds.exists(var))
        _lowerBounds.erase(var);
    if(_upperBounds.exists(var))
        _upperBounds.erase(var);

    if ( !_lowerBounds.exists(_f) ||
         FloatUtils::lt( value, _lowerBounds.get( _f ) ) )
        _lowerBounds[_f] = value;

    _eliminated.insert( var );
    _elements.erase( var );

    checkForFixedPhaseOnAlterationToBounds();

    if ( var == _f || getParticipatingVariables().size() == 1 )
        _removePL = true;
}

void MaxConstraint::tightenPL( Tightening tighten, List<Tightening> &tightenings )
{
    // for now just make sure this is never called
    ASSERT( false );
    if ( tighten._variable == _f )
	{
	    if ( tighten._type == Tightening::LB && FloatUtils::gte( tighten._value, _lowerBounds[_f] ) )
	 	{
		    for ( auto key : _lowerBounds.keys() )
	 		{
			    if ( key == _f ) continue;
			    if ( FloatUtils::areEqual( _lowerBounds.get( key ), tighten._value ) )
	 			{
				    _lowerBounds[key] = tighten._value;
				    tightenings.append( Tightening( key, tighten._value, Tightening::LB ) );
	 			}
	 		}
	 	}
	    else if ( tighten._type == Tightening::UB && FloatUtils::lte( tighten._value, _upperBounds[_f] ) )
	 	{
		    for ( auto key : _upperBounds.keys() )
	 		{
			    if ( key == _f ) continue;
			    if ( FloatUtils::gt( _upperBounds.get( key ), tighten._value ) )
	 			{
				    _upperBounds[key] = tighten._value;
				    tightenings.append( Tightening( key, tighten._value, Tightening::UB ) );
	 			}
	 		}
	 	}
	}
    checkForFixedPhaseOnAlterationToBounds();
}

void MaxConstraint::getAuxiliaryEquations( List<Equation> &/* newEquations */ ) const
{
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
