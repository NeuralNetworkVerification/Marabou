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
#include "MaxConstraint.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"
#include <algorithm>

MaxConstraint::MaxConstraint( unsigned f, const List<unsigned> &elements )
	: PiecewiseLinearConstraint( f )
	, _elements( elements )
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
	
	// // Common PiecewiseLinearConstraint state.
    // clone->_constraintActive = _constraintActive;
    // clone->_assignment = _assignment;
    // clone->_lowerBounds = _lowerBounds;
	// clone->_upperBounds = _upperBounds;
	// clone->_entailedTightenings = _entailedTightenings;	

	// // MaxConstraint-specific state.
    // clone->_maxIndex = _maxIndex;
	// clone->_eliminated = _eliminated;

    return clone;
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
	//Two conditions for _maxIndex to not exist: either _assignment.size()
	//equals to 0, or the only element in _assignment is _f.
	//Otherwise, we only replace _maxIndex if the value of _maxIndex is less
	//than the new value.
		if ( _assignment.size() == 0 || ( _assignment.exists( _f ) &&
	   		 _assignment.size() == 1 ) || _assignment.get( _maxIndex ) < value )
			 _maxIndex = variable;
	}
	_assignment[variable] = value;
}

void MaxConstraint::notifyLowerBound( unsigned variable, double value )
{
	_lowerBounds[variable] = value;
}

void MaxConstraint::notifyUpperBound( unsigned variable, double value )
{
	_upperBounds[variable] = value;
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable );
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
	List<unsigned> temp = _elements;
	temp.append( _f );
	return temp;
}

bool MaxConstraint::satisfied() const
{

	if ( !( _assignment.exists( _f )  &&  _assignment.size() > 1 ) )
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
		PiecewiseLinearCaseSplit maxPhase;
		if ( _assignment.exists( element ) )
		{
            // element - f = 0

            Equation maxEquation;

            maxEquation.addAddend( 1, element );
            maxEquation.addAddend( -1, _f );
            maxEquation.addAuxAddend( 1 );

            Tightening auxUpperBound( 0, 0.0, Tightening::UB );
            Tightening auxLowerBound( 0, 0.0, Tightening::LB );

            maxPhase.storeAuxBoundTightening( auxUpperBound );
            maxPhase.storeAuxBoundTightening( auxLowerBound );

            maxEquation.setScalar( 0 );

            maxPhase.addEquation( maxEquation );

            for ( unsigned other : _elements )
            {
                // element >= other
                if ( element == other )
                    continue;

                Equation gtEquation;

                // other - element + aux = 0
                gtEquation.addAddend( 1, other );
                gtEquation.addAddend( -1, element );
                gtEquation.addAuxAddend( 1 );

                Tightening gtAuxLowerBound( 0, 0.0, Tightening::LB );

                maxPhase.storeAuxBoundTightening( gtAuxLowerBound );

                gtEquation.setScalar( 0 );

                maxPhase.addEquation( gtEquation );
            }
		}
		splits.append( maxPhase );
	}

	return splits;
}

void MaxConstraint::storeState( PiecewiseLinearConstraintState &/* state */ ) const
{
    printf( "Error! Not yet implemented\n" );
    exit( 1 );
}

void MaxConstraint::restoreState( const PiecewiseLinearConstraintState &/* state */ )
{
    printf( "Error! Not yet implemented\n" );
    exit( 1 );
}

PiecewiseLinearConstraintState *MaxConstraint::allocateState() const
{
    printf( "Error! Not yet implemented\n" );
	exit( 1 );
}

bool MaxConstraint::phaseFixed() const
{
    printf( "Error! Not yet implemented\n" );
    exit( 1 );
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    printf( "Error! Not yet implemented\n" );
    exit( 1 );
}

void MaxConstraint::changeVarAssign( unsigned prevVar, unsigned newVar )
{
	ASSERT( participatingVariable( prevVar ) );

	if ( _assignment.exists( prevVar ) )
	{
		_assignment[newVar] = _assignment.get( prevVar );
		_lowerBounds[newVar] = _lowerBounds.get( prevVar  );
		_upperBounds[newVar] = _upperBounds.get( prevVar );
		_assignment.erase( prevVar );
	}

	if ( prevVar == _maxIndex )
		_maxIndex = newVar;

	if ( prevVar == _f )
		_f = newVar;
	else
	{
		_elements.erase( prevVar );
		_elements.append( newVar );
	}
}

void MaxConstraint::eliminateVar( unsigned var, double val )
{
	_eliminated.insert( var );
	val++;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
