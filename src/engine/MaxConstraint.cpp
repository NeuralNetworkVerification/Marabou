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
 ** MaxConstraint implements the following constraint:
 ** _f = Max( e1, e2, ..., eM), where _elements = { e1, e2, ..., eM}
 **
 ** The constraint will refer to elements as its phases, by wrapping the
 ** variable identifiers as PhaseStatus enumeration. Additionally, during
 ** preprocessing one or more phases may be eliminated from the constraint. A
 ** maximum of such constraints is stored locally, and to denote this phase a
 ** special value PhaseStatus::MAX_PHASE_ELIMINATED is used.
 **
 ** MaxConstraint operates in two modes: preprocessing, which uses local bounds
 ** and values, and context-dependent mode which automatically backtracks the
 ** constraint state. A MaxConstraints object enters context-dependent mode upon
 ** invocation of MaxConstraint::initializeCDOs() and it cannot be undone.
 **
 ** Once in the context-dependent mode, MaxConstraint can be used for explicit
 ** exploration of the search space, using the markInfeasible/nextFeasibleCase
 ** methods.
**/

#include "Debug.h"
#include "FloatUtils.h"
#include "IConstraintBoundTightener.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "List.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"
#include <algorithm>

#ifdef _WIN32
#undef max
#undef min
#endif

MaxConstraint::MaxConstraint( unsigned f, const Set<unsigned> &elements )
  : ContextDependentPiecewiseLinearConstraint( elements.size() )
  , _f( f )
	, _elements( elements )
	, _initialElements( elements )
	, _maxLowerBound( FloatUtils::negativeInfinity() )
	, _obsolete( false )
	, _eliminatedVariables( false )
	, _maxValueOfEliminated( FloatUtils::negativeInfinity() )
{
}

MaxConstraint::MaxConstraint( const String &serializedMax )
{
    String constraintType = serializedMax.substring( 0, 3 );
    ASSERT( constraintType == String( "max" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedMax.substring( 4, serializedMax.length() - 4 );
    List<String> values = serializedValues.tokenize( "," );

    auto valuesIter = values.begin();
    unsigned f = atoi( valuesIter->ascii() );
    ++valuesIter;

    Set<unsigned> elements;
    for ( ; *valuesIter != "e" ; ++valuesIter )
        elements.insert( atoi( valuesIter->ascii() ) );

    // Save flag and values indicating eliminated variables
    ++valuesIter;
    bool eliminatedVariableFromString = ( atoi( valuesIter->ascii() ) == 1 );

    ++valuesIter;
    double maxValueOfEliminatedFromString = FloatUtils::negativeInfinity();

    if ( eliminatedVariableFromString )
        maxValueOfEliminatedFromString = std::stod( valuesIter->ascii() );

    *(this) = MaxConstraint( f, elements );
    _eliminatedVariables = eliminatedVariableFromString;
    _maxValueOfEliminated = maxValueOfEliminatedFromString;
}

MaxConstraint::~MaxConstraint()
{
    _elements.clear();
}

PiecewiseLinearFunctionType MaxConstraint::getType() const
{
    return PiecewiseLinearFunctionType::MAX;
}

ContextDependentPiecewiseLinearConstraint *MaxConstraint::duplicateConstraint() const
{
    MaxConstraint *clone = new MaxConstraint( _f, _elements );
    *clone = *this;
    clone->_eliminatedVariables = _eliminatedVariables;
    clone->_maxValueOfEliminated = _maxValueOfEliminated;
    this->initializeDuplicatesCDOs( clone );
    return clone;
}

void MaxConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const MaxConstraint *max = dynamic_cast<const MaxConstraint *>( state );
    ASSERT( nullptr != getContext() );
    ASSERT( nullptr != getActiveStatusCDO() );
    ASSERT( nullptr != getPhaseStatusCDO() );
    ASSERT( getContext() == max->getContext() );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *max;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void MaxConstraint::registerAsWatcher( ITableau *tableau )
{
    for ( unsigned element : _elements )
        tableau->registerToWatchVariable( this, element );

    if ( !_elements.exists( _f ) )
        tableau->registerToWatchVariable( this, _f );
}

void MaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
    for ( unsigned element : _initialElements )
        tableau->unregisterToWatchVariable( this, element );

    if ( !_initialElements.exists( _f ) )
        tableau->unregisterToWatchVariable( this, _f );
}

void MaxConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( ( _elements.exists( _f ) || variable != _f )
         &&
         ( !maxIndexSet() || _assignment.get( getMaxIndex() ) < value ) )
    {
        setMaxIndex( variable );
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
            if ( element == variable || element == _f )
                continue;
            if ( _upperBounds.exists( element ) &&
                 FloatUtils::lt( _upperBounds[element], value ) )
            {
                toRemove.append( element );
            }
        }

        for ( unsigned removeVar : toRemove )
        {
            if ( _cdInfeasibleCases )
                markInfeasible( static_cast<PhaseStatus>( variable ) );
            else
                _elements.erase( removeVar );

            if ( getMaxIndex() == removeVar )
                maxErased = true;
        }
    }

    if ( maxErased )
        resetMaxIndex();

    if ( isActive() && _constraintBoundTightener )
    {
        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // Can focus only on the newly learned bound and possible consequences.
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

    if ( _elements.exists( variable ) && _f != variable && FloatUtils::lt( value, _maxLowerBound ) )
    {
        if ( _cdInfeasibleCases )
            markInfeasible( static_cast<PhaseStatus>( variable ) );
        else
            _elements.erase( variable );
    }

    // If all elements have been eliminated, set the phase of the constraint.
    if ( _elements.size() == 0)
        setMaxIndex( MAX_PHASE_ELIMINATED );

    // There is no need to recompute the max lower bound and max index here.

    if ( isActive() && _constraintBoundTightener )
    {
        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // Can focus only on the newly learned bound and possible consequences.
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

    // Treat the maxValueEliminated as an element
    if ( _eliminatedVariables )
    {
        maxElementLB = FloatUtils::max( _maxValueOfEliminated, maxElementLB );
        maxElementUB = FloatUtils::max( _maxValueOfEliminated, maxElementUB );
    }

    // f_UB and maxElementUB need to be equal. If not, the lower of the two wins.
    if ( FloatUtils::areDisequal( fUB, maxElementUB ) )
    {
        if ( FloatUtils::gt( fUB, maxElementUB ) )
        {
            tightenings.append( Tightening( _f, maxElementUB, Tightening::UB ) );
        }
		else
        {
			// f_UB <= maxElementUB
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

	// f_LB >= maxElementLB & single input element
    else if ( _elements.size() == 1 )
    {
        // Special case: there is only one element. In that case, the tighter lower
        // bound (in this case, f's) wins.
        if ( !_eliminatedVariables )
            tightenings.append( Tightening( *_elements.begin(), fLB, Tightening::LB ) );
        else if ( FloatUtils::gt( fLB, _maxValueOfEliminated ) )
            tightenings.append( Tightening( *_elements.begin(), fLB, Tightening::LB ) );
    }

    // TODO: can we derive additional bounds?
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable );
}

bool MaxConstraint::wereVariablesEliminated() const
{
    return _eliminatedVariables;
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
    List<unsigned> result;
    for ( auto element : _elements )
        result.append( element );

    if ( !_elements.exists( _f ) )
        result.append( _f );
    return result;
}

List<unsigned> MaxConstraint::getElements() const
{
    List<unsigned> result;
    for ( auto element : _elements )
        result.append( element );
    return result;
}

unsigned MaxConstraint::getF() const
{
    return _f;
}

bool MaxConstraint::satisfied() const
{
    if ( !( _assignment.exists( _f ) && _assignment.size() > 0 ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double fValue = _assignment.get( _f );
    double maxValue = FloatUtils::max( _assignment.get( getMaxIndex() ), _maxValueOfEliminated );
    return FloatUtils::areEqual( maxValue, fValue );
}


bool MaxConstraint::isCaseInfeasible( PhaseStatus phase ) const
{
    ASSERT( _cdInfeasibleCases );
    return std::find( _cdInfeasibleCases->begin(), _cdInfeasibleCases->end(), phase ) != _cdInfeasibleCases->end();
}

bool MaxConstraint::isCaseInfeasible( unsigned variable ) const
{
    return isCaseInfeasible( static_cast<PhaseStatus>( variable ) );
}

void MaxConstraint::resetMaxIndex()
{
    double maxValue = FloatUtils::negativeInfinity();
    clearMaxIndex();

    if ( _assignment.empty() ||
         ( _assignment.size() == 1 && !_elements.exists( _f ) && _assignment.begin()->first == _f ) )
    {
        // If none of the variables has been assigned, the max index is
        // not set
        return;
    }
    else
    {
        for ( auto element : _elements )
        {
            if ( _cdInfeasibleCases && isCaseInfeasible( element ) )
                continue;

            if ( _assignment.exists( element ) )
            {
                double elementValue = _assignment[element];

                if ( !maxIndexSet() || elementValue > maxValue )
                {
                    setMaxIndex( element );
                    maxValue = elementValue;
                }
            }
        }

        ASSERT( maxIndexSet() );
    }

    ASSERT( !maxIndexSet() || FloatUtils::isFinite( maxValue ) );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _f ) && ( _assignment.size() > 1 || _eliminatedVariables ) );

    double fValue = _assignment.get( _f );
    double maxVal = FloatUtils::max( _assignment.get( getMaxIndex() ), _maxValueOfEliminated );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations
    //	1. f is greater than maxVal
    //	2. f is less than maxVal
    //  3. f is greater than all variables except one

    if ( FloatUtils::gt( fValue, maxVal ) )
    {
        fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );
        for ( auto elem : _elements )
            if ( !_cdInfeasibleCases || !isCaseInfeasible( elem ) )
                fixes.append( PiecewiseLinearConstraint::Fix( elem, fValue ) );
    }
    else if ( FloatUtils::lt( fValue, maxVal ) )
    {
        fixes.append( PiecewiseLinearConstraint::Fix( _f, maxVal ) );

        unsigned greaterVar;
        unsigned numGreater = 0;
        for ( auto element : _elements )
        {
            if ( _cdInfeasibleCases && !isCaseInfeasible( element ) )
                continue;

            if ( _assignment.exists( element ) && FloatUtils::gt( _assignment[element], fValue ) )
            {
                ++numGreater;
                greaterVar = element;
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

List<PhaseStatus> MaxConstraint::getAllCases() const
{
    if ( _phaseStatus != PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    ASSERT(	_assignment.exists( _f ) );

    List<PhaseStatus> cases;

    if ( !_elements.exists( _f ) )
    {
        for ( unsigned element : _elements )
            cases.append( static_cast<PhaseStatus>( element ) );

        if ( _eliminatedVariables )
            cases.append( MAX_PHASE_ELIMINATED );
    }
    else
    {
        // if elements includes _f, this piecewise linear constraint
        // can immediately be transformed into a conjunction of linear
        // constraints
        cases.append( static_cast<PhaseStatus>( _f ) );
    }

    return cases;
}

List<PiecewiseLinearCaseSplit> MaxConstraint::getCaseSplits() const
{
    if ( phaseFixed() && !_elements.exists( _f ) )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    ASSERT(	_assignment.exists( _f ) );

    List<PiecewiseLinearCaseSplit> splits;

    if ( !_elements.exists( _f ) )
    {
        for ( unsigned element : _elements )
        {
            if ( !_cdInfeasibleCases || !isCaseInfeasible( element ) )
                splits.append( getSplit( element ) );
        }

        if ( _eliminatedVariables )
        {
            // Consider also f = maxEliminated
            PiecewiseLinearCaseSplit phaseOfEliminatedIsMax;
            phaseOfEliminatedIsMax.storeBoundTightening( Tightening( _f, _maxValueOfEliminated, Tightening::LB ) );
            phaseOfEliminatedIsMax.storeBoundTightening( Tightening( _f, _maxValueOfEliminated, Tightening::UB ) );

            for ( unsigned element : _elements )
            {
                if ( element == _f )
                    continue;

                // element <= maxEliminated
                phaseOfEliminatedIsMax.storeBoundTightening( Tightening( element, _maxValueOfEliminated, Tightening::UB ) );
            }
            splits.append( phaseOfEliminatedIsMax );
        }
    }
    else
    {
        // if elements includes _f, this piecewise linear constraint
        // can immediately be transformed into a conjunction of linear
        // constraints
        splits.append( getSplit( _f ) );
    }

    return splits;
}

bool MaxConstraint::phaseFixed() const
{
    if ( _elements.exists( _f ) )
        return true;
    if ( _elements.size() == 1 )
    {
        if ( !_eliminatedVariables )
            return true;

        unsigned singleVarLeft = *_elements.begin();
        if ( _lowerBounds.exists( singleVarLeft ) && FloatUtils::gte( _lowerBounds[singleVarLeft], _maxValueOfEliminated ) )
            return true;

        if ( _upperBounds.exists( singleVarLeft ) && FloatUtils::lte( _upperBounds[singleVarLeft], _maxValueOfEliminated ) )
            return true;
    }

    // The case where all variables are eliminated
    if ( _elements.empty() && _eliminatedVariables )
        return true;

    return false;
}

bool MaxConstraint::isImplication() const
{
    return _elements.exists( _f ) || numFeasibleCases() == 1u;
}


PiecewiseLinearCaseSplit MaxConstraint::getImpliedCaseSplit() const
{
    ASSERT( isImplication() );

    ASSERT( phaseFixed() );

    PhaseStatus phase = getPhaseStatus();

    ASSERT ( phase != PHASE_NOT_FIXED );

    if ( phase == MAX_PHASE_ELIMINATED )
    {
        PiecewiseLinearCaseSplit phaseOfEliminatedIsMax;
        phaseOfEliminatedIsMax.storeBoundTightening( Tightening( _f, _maxValueOfEliminated, Tightening::LB ) );
        phaseOfEliminatedIsMax.storeBoundTightening( Tightening( _f, _maxValueOfEliminated, Tightening::UB ) );
        return phaseOfEliminatedIsMax;
    }

    return getSplit( phase ); // Handles the special case of _f being the phase
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

PiecewiseLinearCaseSplit MaxConstraint::getCaseSplit( PhaseStatus phase ) const
{
    return getSplit( static_cast<unsigned>( phase ) );
}

PiecewiseLinearCaseSplit MaxConstraint::getSplit( unsigned argMax ) const
{
    ASSERT( _assignment.exists( argMax ) );
    PiecewiseLinearCaseSplit maxPhase;

    if ( argMax != _f )
    {
        // maxArg - f = 0
        Equation maxEquation( Equation::EQ );
        maxEquation.addAddend( 1, argMax );
        maxEquation.addAddend( -1, _f );
        maxEquation.setScalar( 0 );
        maxPhase.addEquation( maxEquation );
    }

    // Store bound tightenings as well
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
            {
                maxPhase.storeBoundTightening( Tightening( other, _upperBounds[argMax], Tightening::UB ) );
            }
        }
    }

    if ( _eliminatedVariables )
    {
        // argMax >= maxValueOfEliminated
        maxPhase.storeBoundTightening( Tightening( argMax, _maxValueOfEliminated, Tightening::LB ) );
    }

    return maxPhase;
}

void MaxConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    _lowerBounds[newIndex] = _lowerBounds[oldIndex];
    _upperBounds[newIndex] = _upperBounds[oldIndex];

    if ( oldIndex == _f )
        _f = newIndex;

    if ( _elements.exists( oldIndex ) )
    {
        _elements.erase( oldIndex );
        _elements.insert( newIndex );
    }
}

bool MaxConstraint::constraintObsolete() const
{
    return _obsolete;
}

void MaxConstraint::eliminateVariable( unsigned var, double value )
{
    // First elimination does not remove number of cases, since it
    // simultaneously adds MAX_PHASE_ELIMINATED
    if ( _eliminatedVariables )
    {
        ASSERT( _numCases > 1u ); // MAX_PHASE_ELIMINATED cannot be removed
        --_numCases;
    }

    _eliminatedVariables = true;
    _maxValueOfEliminated = FloatUtils::max( value, _maxValueOfEliminated );
    _maxLowerBound = FloatUtils::max( _maxLowerBound, _maxValueOfEliminated );

    _elements.erase( var );
    if ( var == _f )
        _obsolete = true;
    if ( getParticipatingVariables().size() == 1 )
    {
        if ( !_eliminatedVariables )
            _obsolete = true;

        // Variables were eliminated
        unsigned singleVariableLeft = *_elements.begin();
        if ( FloatUtils::gte( _lowerBounds[singleVariableLeft], _maxValueOfEliminated ) )
            _obsolete = true;

        if ( FloatUtils::lte( _upperBounds[singleVariableLeft], _maxValueOfEliminated ) )
            _obsolete = true;
    }
}

void MaxConstraint::addAuxiliaryEquations( InputQuery &inputQuery )
{
    for ( auto element : _elements )
    {
        // If element is equal to _f, skip this step.
        // The reason is to avoid adding equations like `1.00x00 -1.00x00 -1.00x01 = 0.00`.
        if ( element == _f )
            continue;

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

    // Special delimiter ",e" represents elimination flag and variables
    output += Stringf( ",e" );
    output += Stringf( ",%u", _eliminatedVariables ? 1 : 0 );
    if ( _eliminatedVariables )
        output += Stringf( ",%f", _maxValueOfEliminated );
    else
        // Will be ignored in any case
        output += Stringf( ",%u", 0 );

    return output;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
