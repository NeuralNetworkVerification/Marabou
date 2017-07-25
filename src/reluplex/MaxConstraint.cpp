#include "Debug.h"
#include "FloatUtils.h"
#include "FreshVariables.h"
#include "PiecewiseLinearCaseSplit.h"
#include "MaxConstraint.h"
#include "ReluplexError.h"
#include <algorithm>  

MaxConstraint::MaxConstraint( unsigned f, List<unsigned> elems )
	: _f( f )
	, _elems( elems )
{
	
}

void MaxConstraint::registerAsWatcher( ITableau *tableau )
{
	tableau->registerToWatchVariable( this, _f );
	for ( unsigned elem : _elems )
		tableau->registerToWatchVariable( this, elem );		
}

void MaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
	tableau->unregisterToWatchVariable( this, _f );
	for ( unsigned elem : _elems )
		tableau->unregisterToWatchVariable( this, elem );		
}

void MaxConstraint::notifyVariableValue( unsigned variable, double value )
{
	_assignment[variable] = value;
	
	if ( variable != _f )
	{
		if ( !_assignment.exists( _maxIndex ) || ( _assignment.exists( _maxIndex ) && _assignment.get( _maxIndex ) < value ) )
			_maxIndex = variable;
	}
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{	
	for ( unsigned elem : _elems )
	{
		if ( elem == variable ) 
			return true;
	}
	return variable == _f; 
}

List<unsigned> MaxConstraint::getParticiatingVariables() const
{
	List<unsigned> temp = _elems;
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
	if ( !( _assignment.exists( _f )  && _assignment.exists( _maxIndex ) ) ) 
		throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );
			
	double fValue = _assignment.get( _f );
	

	return FloatUtils::areEqual( _assignment.get( _maxIndex ), fValue );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
	ASSERT( !satisfied() );
	ASSERT(	_assignment.exists( _f ) );
	ASSERT( _assignment.exists( _maxIndex ) );
	
	double fValue = _assignment.get( _f );
	double maxVal = _assignment.get( _maxIndex );

	List<PiecewiseLinearConstraint::Fix> fixes;

	//Possible violations
	//	1. f is greater than maxVal
	//	2. f is less than maxVal

	if ( FloatUtils::gt( fValue, maxVal ) )
	{
		fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
		fixes.append( PiecewiseLinearConstraint::Fix( _maxIndex, fValue ) );
	}
	else if ( FloatUtils::lt( fValue, maxVal ) )
	{
		fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
		fixes.append( PiecewiseLinearConstraint::Fix( _maxIndex, fValue ) );
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
	List<PiecewiseLinearCaseSplit> splits;

	PiecewiseLinearCaseSplit maxPhase;

	for ( unsigned elem : _elems )
	{
		if ( _assignment.exists( elem ) )
		{
				//elem - f = 0
				unsigned auxVariable = FreshVariables::getNextVariable();

				Equation maxEquation;

				maxEquation.addAddend( 1, elem );
				maxEquation.addAddend( -1, _f );
				maxEquation.addAddend( -1, auxVariable );

				PiecewiseLinearCaseSplit::Bound auxUpperBound( auxVariable, PiecewiseLinearCaseSplit::Bound::UPPER, 0.0 );
				PiecewiseLinearCaseSplit::Bound auxLowerBound( auxVariable, PiecewiseLinearCaseSplit::Bound::LOWER, 0.0 );

				maxPhase.storeBoundTightening( auxUpperBound );
				maxPhase.storeBoundTightening( auxLowerBound );

				maxEquation.markAuxiliaryVariable( auxVariable );

				maxEquation.setScalar( 0 );

				maxPhase.addEquation( maxEquation );

				for ( unsigned other : _elems )
				{
						//elem >= other
						if ( elem == other ) continue;

						unsigned gtAuxVariable = FreshVariables::getNextVariable();

						Equation gtEquation;

						//other - element + aux = 0
						gtEquation.addAddend( 1, other );
						gtEquation.addAddend( -1, elem );
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
