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
#include "MaxConstraint.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"
#include <algorithm>

MaxConstraint::MaxConstraint( unsigned f, const List<unsigned> &elements )
	: _f( f )
	, _elements( elements )
{
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
	_assignment[variable] = value;

	if ( variable != _f )
	{
        // Guy: if the first disjunct meant to check whether the _assignment is empty? If so, please use _assignment.empty()
		if ( !_assignment.exists( _maxIndex ) || ( _assignment.exists( _maxIndex ) && _assignment.get( _maxIndex ) < value ) )
			_maxIndex = variable;
	}
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable );
}

List<unsigned> MaxConstraint::getParticiatingVariables() const
{
	List<unsigned> temp = _elements;
	temp.append( _f );
	return temp;
}

bool MaxConstraint::satisfied() const
{
	/*bool exists = false;
	for ( unsigned elem : _elems )
	{
		if ( _assignment.exists( elem ) )
		{
			exists = true;
			break;
		}
	}*/

    // Again, if the second disjunct is for checking whether _assignment is empty, use _assignment.empty()
	if ( !( _assignment.exists( _f )  && _assignment.exists( _maxIndex ) ) )
		throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

	double fValue = _assignment.get( _f );
	return FloatUtils::areEqual( _assignment.get( _maxIndex ), fValue );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
	ASSERT( !satisfied() );
	ASSERT(	_assignment.exists( _f ) );
   // Again, if this assertion is for checking whether _assignment is empty, use _assignment.empty()
	ASSERT( _assignment.exists( _maxIndex ) );

	double fValue = _assignment.get( _f );
	double maxVal = _assignment.get( _maxIndex );

	List<PiecewiseLinearConstraint::Fix> fixes;

	// Possible violations
	//	1. f is greater than maxVal
	//	2. f is less than maxVal

	if ( FloatUtils::gt( fValue, maxVal ) )
	{
		fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
		fixes.append( PiecewiseLinearConstraint::Fix( _maxIndex, fValue ) );

        // Guy: we can propose to increase any of the variables, not just _maxIndex, as a fix here.
	}
	else
	{
        // fValue is less than maxVal
		fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
		fixes.append( PiecewiseLinearConstraint::Fix( _maxIndex, fValue ) );
        // Guy: I think the above fix is wrong. What if fVal = 5 and the elements' values are 6 and 7?
        // This will reduce 7 to 5, but 6 will still be greater than 5, right?

		/*for ( unsigned elem : _elems )
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
	PiecewiseLinearCaseSplit maxPhase;

	for ( unsigned element : _elements )
	{
		if ( _assignment.exists( element ) )
		{
            // element - f = 0
            unsigned auxVariable = FreshVariables::getNextVariable();

            Equation maxEquation;

            maxEquation.addAddend( 1, element );
            maxEquation.addAddend( -1, _f );
            // Guy: why set auxVariable's coefficient to -1? +1 seems more natural?
            maxEquation.addAddend( -1, auxVariable );

            PiecewiseLinearCaseSplit::Bound auxUpperBound( auxVariable, PiecewiseLinearCaseSplit::Bound::UPPER, 0.0 );
            PiecewiseLinearCaseSplit::Bound auxLowerBound( auxVariable, PiecewiseLinearCaseSplit::Bound::LOWER, 0.0 );

            maxPhase.storeBoundTightening( auxUpperBound );
            maxPhase.storeBoundTightening( auxLowerBound );

            maxEquation.markAuxiliaryVariable( auxVariable );

            maxEquation.setScalar( 0 );

            maxPhase.addEquation( maxEquation );

            for ( unsigned other : _elements )
            {
                // element >= other
                if ( element == other )
                    continue;

                unsigned gtAuxVariable = FreshVariables::getNextVariable();

                Equation gtEquation;

                // other - element + aux = 0
                gtEquation.addAddend( 1, other );
                gtEquation.addAddend( -1, element );
                gtEquation.addAddend( 1, gtAuxVariable );

                PiecewiseLinearCaseSplit::Bound gtAuxLowerBound( gtAuxVariable, PiecewiseLinearCaseSplit::Bound::LOWER, 0.0 );

                maxPhase.storeBoundTightening( gtAuxLowerBound );

                gtEquation.markAuxiliaryVariable( gtAuxVariable );

                gtEquation.setScalar( 0 );

                maxPhase.addEquation( gtEquation );
            }
		}
	}

	splits.append( maxPhase );

	return splits;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
