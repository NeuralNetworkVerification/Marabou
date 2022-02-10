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
 ** See the description of the class in MaxConstraint.h.
 **/

#include "MaxConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "IConstraintBoundTightener.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "List.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"

#include <algorithm>

#ifdef _WIN32
#undef max
#undef min
#endif

MaxConstraint::MaxConstraint( unsigned f, const Set<unsigned> &elements )
    : PiecewiseLinearConstraint( elements.size() )
    , _f( f )
    , _elements( elements )
    , _initialElements( elements )
    , _obsolete( false )
    , _maxLowerBound( FloatUtils::negativeInfinity() )
    , _hasFeasibleEliminatedVariables( false )
    , _maxValueOfEliminatedVariables( FloatUtils::negativeInfinity() )
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
    for ( ; *valuesIter != "e"; ++valuesIter )
        elements.insert( atoi( valuesIter->ascii() ) );

    // Save flag and values indicating eliminated variables
    ++valuesIter;
    bool eliminatedVariableFromString = ( atoi( valuesIter->ascii() ) == 1 );

    ++valuesIter;
    double maxValueOfEliminatedFromString = FloatUtils::negativeInfinity();

    if ( eliminatedVariableFromString )
        maxValueOfEliminatedFromString = std::stod( valuesIter->ascii() );

    *(this) = MaxConstraint( f, elements );
    _hasFeasibleEliminatedVariables = eliminatedVariableFromString;
    _maxValueOfEliminatedVariables = maxValueOfEliminatedFromString;
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
    clone->_hasFeasibleEliminatedVariables
        = _hasFeasibleEliminatedVariables;
    clone->_maxValueOfEliminatedVariables = _maxValueOfEliminatedVariables;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void MaxConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const MaxConstraint *max = dynamic_cast<const MaxConstraint *>( state );

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

    for ( unsigned element : _elements )
        tableau->registerToWatchVariable( this, _elementToAux[element] );

    if ( !_elements.exists( _f ) )
        tableau->registerToWatchVariable( this, _f );
}

void MaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
    for ( unsigned element : _initialElements )
        tableau->unregisterToWatchVariable( this, element );

    for ( unsigned element : _initialElements )
        tableau->unregisterToWatchVariable( this, _elementToAux[element] );

    if ( !_initialElements.exists( _f ) )
        tableau->unregisterToWatchVariable( this, _f );
}

void MaxConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

void MaxConstraint::notifyLowerBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incLongAttribute
            ( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( existsLowerBound( variable ) &&
         !FloatUtils::gt( value, getLowerBound( variable ) ) )
        return;

    setLowerBound( variable, value );

    /*
      See if we can eliminate any cases.
    */
    if ( _auxToElement.exists( variable ) )
    {
        // The case that this variable is an aux variable.
        // We can eliminate the corresponding case if the aux variable is
        // positive.
        if ( FloatUtils::isPositive( value ) )
            _elements.erase( _auxToElement[variable] );
    }
    else
    {
        // If the variable is either in _element or _f. The only case that this is
        // going to eliminate case is that the new lowerBound is greater than the
        // _maxLowerBound.
        // We update _maxLowerBound, and go through each element to see if it
        // can be eliminated.
        if ( FloatUtils::gt( value, _maxLowerBound ) )
        {
            _maxLowerBound = value;
            List<unsigned> toRemove;
            for ( auto element : _elements )
            {
                if ( element == variable || element == _f )
                    continue;
                if ( existsUpperBound( element ) &&
                     FloatUtils::lt( getUpperBound( element ), value ) )
                {
                    toRemove.append( element );
                }
            }
            for ( unsigned removeVar : toRemove )
            {
                if ( _cdInfeasibleCases )
                    markInfeasible( variableToPhase( variable ) );
                else
                    _elements.erase( removeVar );
            }
        }
    }

    /*
      Now do some bound tightening.
    */
    if ( isActive() && _constraintBoundTightener )
    {
        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // Can focus only on the newly learned bound and possible consequences.
        List<Tightening> tightenings;
        getEntailedTightenings( tightenings );
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
                _constraintBoundTightener->
                    registerTighterLowerBound( tightening._variable, tightening._value );
            else if ( tightening._type == Tightening::UB )
                _constraintBoundTightener->
                    registerTighterUpperBound( tightening._variable, tightening._value );
        }
    }
}

void MaxConstraint::notifyUpperBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( existsUpperBound( variable ) && !FloatUtils::lt( value, getUpperBound( variable ) ) )
        return;

    setUpperBound( variable, value );

    /*
      See if we can eliminate any cases.
    */
    if ( _auxToElement.exists( variable ) )
    {
        // The case that this variable is an aux variable.
        // We can eliminate the corresponding case if the aux variable is
        // positive.
        if ( FloatUtils::isZero( value ) )
            _elements.erase( _auxToElement[variable] );
    }
    else
    {
        // If the variable is either in _element or _f. The only case that this is
        // going to eliminate case is that the new lowerBound is greater than the
        // _maxLowerBound.
        // We update _maxLowerBound, and go through each element to see if it
        // can be eliminated.
        if ( FloatUtils::gt( value, _maxLowerBound ) )
        {
            _maxLowerBound = value;
            List<unsigned> toRemove;
            for ( auto element : _elements )
            {
                if ( element == variable || element == _f )
                    continue;
                if ( existsUpperBound( element ) &&
                     FloatUtils::lt( getUpperBound( element ), value ) )
                {
                    toRemove.append( element );
                }
            }
            for ( unsigned removeVar : toRemove )
            {
                if ( _cdInfeasibleCases )
                    markInfeasible( variableToPhase( variable ) );
                else
                    _elements.erase( removeVar );
            }
        }
    }

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
    double fLB = existsLowerBound( _f ) ? getLowerBound( _f ) : FloatUtils::negativeInfinity();
    double fUB = existsUpperBound( _f ) ? getUpperBound( _f ) : FloatUtils::infinity();

    // Compute the maximal bounds (lower and upper) for the elements
    double maxElementLB = FloatUtils::negativeInfinity();
    double maxElementUB = FloatUtils::negativeInfinity();

    for ( const auto &element : _elements )
    {
        if ( existsLowerBound( element ) )
            maxElementLB = FloatUtils::max( getLowerBound( element ), maxElementLB );

        if ( !existsUpperBound( element ) )
            maxElementUB = FloatUtils::infinity();
        else
            maxElementUB = FloatUtils::max( getUpperBound( element ), maxElementUB );
    }

    // Treat the maxValueEliminated as an element
    if ( _hasFeasibleEliminatedVariables )
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
                if ( !existsUpperBound( element ) || FloatUtils::gt( getUpperBound( element ), fUB ) )
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
        if ( !_eliminatedVariables || FloatUtils::gt( fLB, _maxValueOfEliminated ) )
            tightenings.append( Tightening( *_elements.begin(), fLB, Tightening::LB ) );
    }
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable ) ||
        _auxToElement.exists( variable );
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
    List<unsigned> result;

    for ( auto element : _elements )
        result.append( element );

    for ( auto pair : _auxs )
        result.append( pair.second );

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

    auto byAssignment = [&](const unsigned& a, const unsigned& b) {
                            return assignment[a] < assignment[b];
                        };
    unsigned maxValue =  _assignment.get( *std::max_element( _elements.begin(),
                                                             _elements.end(),
                                                             byAssignment ) );
    double fValue = _assignment.get( _f );
    double maxValue = FloatUtils::max( maxValue, _maxValueOfEliminated );
    return FloatUtils::areEqual( maxValue, fValue );
}

bool MaxConstraint::isCaseInfeasible( unsigned variable ) const
{
    return PiecewiseLinearConstraint::isCaseInfeasible( variableToPhase( variable ) );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getSmartFixes( ITableau * ) const
{
    return getPossibleFixes();
}

List<PhaseStatus> MaxConstraint::getAllCases() const
{
    if ( phaseFixed() )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PhaseStatus> cases;
    if ( !_elements.exists( _f ) )
    {
        for ( unsigned element : _elements )
            cases.append( variableToPhase( element ) );

        if ( _hasFeasibleEliminatedVariables )
            cases.append( MAX_PHASE_ELIMINATED );
    }
    else
    {
        // if elements includes _f, this piecewise linear constraint
        // can immediately be transformed into a conjunction of linear
        // constraints
        cases.append( variableToPhase( _f ) );
    }

    return cases;
}

List<PiecewiseLinearCaseSplit> MaxConstraint::getCaseSplits() const
{
    if ( phaseFixed() && !_elements.exists( _f ) )
        throw MarabouError
            ( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;

    if ( !_elements.exists( _f ) )
    {
        for ( unsigned element : _elements )
        {
            if ( !_cdInfeasibleCases || !isCaseInfeasible( element ) )
                splits.append( getSplit( element ) );
        }

        if ( _hasFeasibleEliminatedVariables )
        {
            // Consider also f = maxEliminated
            PiecewiseLinearCaseSplit phaseOfEliminatedIsMax;
            phaseOfEliminatedIsMax.storeBoundTightening
                ( Tightening( _f, _maxValueOfEliminatedVariables,
                              Tightening::LB ) );
            phaseOfEliminatedIsMax.storeBoundTightening
                ( Tightening( _f, _maxValueOfEliminatedVariables,
                              Tightening::UB ) );

            for ( unsigned element : _elements )
            {
                if ( element == _f )
                    continue;

                // element <= maxEliminated
                phaseOfEliminatedIsMax.storeBoundTightening
                    ( Tightening( element, _maxValueOfEliminated,
                                  Tightening::UB ) );
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
        if ( existsLowerBound( singleVarLeft ) &&
             FloatUtils::gte( getLowerBound( singleVarLeft ),
                              _maxValueOfEliminated ) )
            return true;

        if ( existsUpperBound( singleVarLeft ) &&
             FloatUtils::lte( getUpperBound( singleVarLeft ),
                              _maxValueOfEliminated ) )
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
    ASSERT( phaseFixed() );

    PhaseStatus phase = getPhaseStatus();

    ASSERT( phase != PHASE_NOT_FIXED );

    if ( phase == MAX_PHASE_ELIMINATED )
    {
        PiecewiseLinearCaseSplit phaseOfEliminatedIsMax;
        phaseOfEliminatedIsMax.storeBoundTightening
            ( Tightening( _f, _maxValueOfEliminatedVariables, Tightening::LB ) );
        phaseOfEliminatedIsMax.storeBoundTightening
            ( Tightening( _f, _maxValueOfEliminatedVariables, Tightening::UB ) );
        return phaseOfEliminatedIsMax;
    }

    return getCaseSplit( phase ); // Handles the special case of _f being the phase
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

PiecewiseLinearCaseSplit MaxConstraint::getCaseSplit( PhaseStatus phase ) const
{
    return getSplit( phaseToVariable( phase ) );
}

PiecewiseLinearCaseSplit MaxConstraint::getSplit( unsigned argMax ) const
{
    PiecewiseLinearCaseSplit maxPhase;

    if ( argMax != _f )
    {
        // We had f - argMax = aux and
        maxPhase.storeBoundTightening( Tightening( _elementToAux[argMax], 0,
                                                       Tightening::UB ) );
    }

    // Store bound tightenings as well
    // go over all other elements;
    // their upper bound cannot exceed upper bound of argmax
    for ( unsigned other : _elements )
    {
        if ( argMax == other )
            continue;

        if ( existsUpperBound( argMax ) )
        {
            if ( !existsUpperBound( other ) ||
                 FloatUtils::gt( getUpperBound( other ), getUpperBound( argMax ) ) )
            {
                maxPhase.storeBoundTightening
                    ( Tightening( other, getUpperBound( argMax ), Tightening::UB ) );
            }
        }
    }

    if ( _eliminatedVariables )
    {
        // argMax >= maxValueOfEliminated
        maxPhase.storeBoundTightening
            ( Tightening( argMax, _maxValueOfEliminated, Tightening::LB ) );
    }

    return maxPhase;
}

void MaxConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    _lowerBounds[newIndex] = _lowerBounds[oldIndex];
    _upperBounds[newIndex] = _upperBounds[oldIndex];

    if ( oldIndex == _f )
        _f = newIndex;
    else if ( _elements.exists( oldIndex ) )
    {
        _elements.erase( oldIndex );
        _elements.insert( newIndex );

        unsigned auxVar = _elementToAux[oldIndex];
        _elementToAux.erase( oldIndex );
        _elementToAux[newIndex] = auxVar;
        _auxToElement[auxVar] = newIndex;
    }
    else
    {
        ASSERT( _auxsInUse && _auxToElement.exists( oldIndex ) );
        unsigned element = _auxToElement[oldIndex];
        _elementToAux[element] = newIndex;
        _auxToElement.erase( oldIndex );
        _auxToElement[newIndex] = element;
    }
}

bool MaxConstraint::constraintObsolete() const
{
    return _obsolete;
}

void MaxConstraint::eliminateVariable( unsigned var, double value )
{
    if ( var == _f )
    {
        _obsolete = true;
        return;
    }
    else if ( _elements.exists( var ) )
    {
        _elements.erase( var );
        unsigned aux = _elementToAux[var];
        _elementToAux.erase( var );
        _auxToElement.erase( aux );

        _maxValueOfEliminatedVariables = FloatUtils::max
            ( value, _maxValueOfEliminatedVariables );

        _hasFeasibleEliminatedVariables =
            FloatUtils::gte( _maxValueOfEliminatedVariables, _maxLowerBound );
    }
    else
    {
        // Must be aux var
        ASSERT( _auxsInUse && _auxToElement.exists( var ) &&
                FloatUtils::gte( value, 0 ) );
        unsigned element = _auxToElement[var];
        _elements.erase( var );
        unsigned aux = _elementToAux[var];
        _elementToAux.erase( var );
        _auxToElement.erase( aux );

        if ( FloatUtils::isZero( value ) )
        {
            // The constraint must be fixed.
            _obsolete = true;
        }
    }

    if ( _elements.size() == 0 ||
         ( _elements.size() == 1 && !_hasFeasibleEliminatedVariables ) )
        _obsolete = true;
}

void MaxConstraint::transformToUseAuxVariablesIfNeeded( InputQuery &inputQuery )
{
    for ( auto element : _elements )
    {
        // If element is equal to _f, skip this step.
        // The reason is to avoid adding equations like `1.00x00 -1.00x00 -1.00x01 = 0.00`.
        if ( element == _f )
        {
            _obsolete = true;
            continue;
        }

        // Create an aux variable
        unsigned auxVariable = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( auxVariable + 1 );

        // f >= element, or f - element - aux = 0, for non-negative aux
        Equation equation( Equation::EQ );
        equation.addAddend( 1.0, _f );
        equation.addAddend( -1.0, element );
        equation.addAddend( -1.0, auxVariable );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        // Set the bounds for the aux variable
        inputQuery.setLowerBound( auxVariable, 0 );

        _elementToAux[element] = auxVariable;
        _auxToElement[auxVariable] = element;
    }
}

void MaxConstraint::getCostFunctionComponent( LinearExpression &cost,
                                              PhaseStatus phase ) const
{
    // If the constraint is not active or is fixed, it contributes nothing
    if( !isActive() || phaseFixed() )
        return;

    // This should not be called when the linear constraints have
    // not been satisfied
    ASSERT( !haveOutOfBoundVariables() );

    // The soundness of the SoI component assumes that the constraints
    // f >= element is added.
    DEBUG({
            for ( const auto &element : _elements )
            {
                ASSERT( FloatUtils::gte( _assignment.get( _f ),
                                         _assignment.get( element ) ) );
            }
            if ( _eliminatedVariables )
                ASSERT( FloatUtils::gte( _assignment.get( _f ),
                                         _maxValueOfEliminated ) );
        });

    if ( phase == MAX_PHASE_ELIMINATED )
    {
        // The cost term corresponding to this phase is f - maxValueOfEliminated.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        cost._addends[_f] = cost._addends[_f] + 1;
        cost._constant -= _maxValueOfEliminatedVariables;
    }
    else
    {
        unsigned element = phaseToVariable( phase );
        unsigned aux = _elementToAux[element];
        if ( !cost._addends.exists( aux ) )
            cost._addends[aux] = 0;
        cost._addends[aux] += 1;
    }
}

PhaseStatus MaxConstraint::getPhaseStatusInAssignment( const Map<unsigned, double>
                                                       &assignment ) const
{
    auto byAssignment = [&](const unsigned& a, const unsigned& b) {
                            return assignment[a] < assignment[b];
                        };
    unsigned largestVariable =  *std::max_element( _elements.begin(),
                                                   _elements.end(),
                                                   byAssignment );
    if ( FloatUtils::lt( assignment[largestVariable], assignment[_f] ) )
        return MAX_PHASE_ELIMINATED;
    else
        return variableToPhase( largestVariable );
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

bool MaxConstraint::haveOutOfBoundVariables() const
{
    double fValue = _assignment.get( _f );
    if ( FloatUtils::gt( getLowerBound( _f ), fValue ) ||
         FloatUtils::lt( getUpperBound( _f ), fValue ) )
        return true;

    for ( const auto &element : _elements )
    {
        double value = _assignment.get( element );
        if ( FloatUtils::gt( getLowerBound( element ), value ) ||
             FloatUtils::lt( getUpperBound( element ), value ) )
        return true;
    }
    return false;
}

bool MaxConstraint::allInputElementsFeasible() const
{
    for ( const auto &element : _elements )
    {

    }
}
