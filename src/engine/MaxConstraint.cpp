/*********************                                                        */
/*! \file MaxConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Derek Huang, Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in MaxConstraint.h.
 **/

#include "MaxConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "List.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"
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
    , _haveFeasibleEliminatedPhases( false )
    , _maxValueOfEliminatedPhases( FloatUtils::negativeInfinity() )
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

    *( this ) = MaxConstraint( f, elements );
    _haveFeasibleEliminatedPhases = eliminatedVariableFromString;
    _maxValueOfEliminatedPhases = maxValueOfEliminatedFromString;
}

MaxConstraint::~MaxConstraint()
{
    _elementToTighteningRow.clear();
    _elementToTableauAux.clear();
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
    {
        tableau->registerToWatchVariable( this, element );
        if ( _elementToAux.exists( element ) )
            tableau->registerToWatchVariable( this, _elementToAux[element] );
    }

    if ( !_elements.exists( _f ) )
        tableau->registerToWatchVariable( this, _f );
}

void MaxConstraint::unregisterAsWatcher( ITableau *tableau )
{
    for ( unsigned element : _initialElements )
    {
        tableau->unregisterToWatchVariable( this, element );
        if ( _elementToAux.exists( element ) )
            tableau->unregisterToWatchVariable( this, _elementToAux[element] );
    }

    if ( !_initialElements.exists( _f ) )
        tableau->unregisterToWatchVariable( this, _f );
}

void MaxConstraint::notifyLowerBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr )
    {
        if ( existsLowerBound( variable ) && !FloatUtils::gt( value, getLowerBound( variable ) ) )
            return;

        setLowerBound( variable, value );
    }

    bool proofs = _boundManager && _boundManager->shouldProduceProofs();

    /*
      See if we can eliminate any cases.
    */
    if ( _auxToElement.exists( variable ) && FloatUtils::isPositive( value ) )
    {
        // The case that this variable is an aux variable.
        // We can eliminate the corresponding case if the aux variable is
        // positive.
        eliminateCase( _auxToElement[variable] );
    }
    else if ( variable == _f || _elements.exists( variable ) )
    {
        // If the variable is either in _element or _f, there will be eliminated
        // cases when the new lowerBound is greater than the _maxLowerBound.
        if ( FloatUtils::gt( value, _maxLowerBound ) )
        {
            _maxLowerBound = value;
            List<unsigned> toRemove;
            for ( auto element : _elements )
            {
                if ( element == variable )
                    continue;
                if ( existsUpperBound( element ) &&
                     FloatUtils::lt( getUpperBound( element ), value ) )
                {
                    toRemove.append( element );
                }
            }
            for ( unsigned removeVar : toRemove )
                eliminateCase( removeVar );

            _haveFeasibleEliminatedPhases =
                FloatUtils::lte( _maxLowerBound, _maxValueOfEliminatedPhases );
        }
    }

    if ( phaseFixed() )
        _phaseStatus = ( _haveFeasibleEliminatedPhases ? MAX_PHASE_ELIMINATED
                                                       : variableToPhase( *_elements.begin() ) );

    if ( isActive() && _boundManager )
    {
        if ( proofs )
            for ( const auto &element : _elements )
                createElementTighteningRow( element );

        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // Can focus only on the newly learned bound and possible consequences.
        List<Tightening> tightenings;
        getEntailedTightenings( tightenings );
        applyTightenings( tightenings );
    }
}

void MaxConstraint::notifyUpperBound( unsigned variable, double value )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr )
    {
        if ( existsUpperBound( variable ) && !FloatUtils::lt( value, getUpperBound( variable ) ) )
            return;

        setUpperBound( variable, value );
    }

    bool proofs = _boundManager && _boundManager->shouldProduceProofs();

    /*
      See if we can eliminate any cases.
    */
    if ( _auxToElement.exists( variable ) )
    {
        /* The case that this variable is an aux variable.
           If the aux variable is 0. This means f - auxToElement[aux] = aux = 0.
           We can eliminate all other cases and set
           _haveFeasibleEliminatedPhases to false;
        */
        if ( FloatUtils::isZero( value ) )
        {
            unsigned currentElement = _auxToElement[variable];
            Set<unsigned> toRemove = _elements;
            toRemove.erase( currentElement );
            for ( const auto &element : toRemove )
                eliminateCase( element );
            _haveFeasibleEliminatedPhases = false;
        }
    }
    else if ( _elements.exists( variable ) )
    {
        // If the upper bound of this variable is below the _maxLowerBound,
        // this case is infeasible.
        if ( FloatUtils::lt( value, _maxLowerBound ) )
            eliminateCase( variable );
    }

    if ( phaseFixed() )
        _phaseStatus = ( _haveFeasibleEliminatedPhases ? MAX_PHASE_ELIMINATED
                                                       : variableToPhase( *_elements.begin() ) );

    // There is no need to recompute the max lower bound and max index here.

    if ( isActive() && _boundManager )
    {
        if ( proofs )
            for ( const auto &element : _elements )
                createElementTighteningRow( element );

        // TODO: optimize this. Don't need to recompute ALL possible bounds,
        // Can focus only on the newly learned bound and possible consequences.
        List<Tightening> tightenings;
        getEntailedTightenings( tightenings );
        applyTightenings( tightenings );
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

    maxElementLB = FloatUtils::max( _maxValueOfEliminatedPhases, maxElementLB );
    maxElementUB = FloatUtils::max( _maxValueOfEliminatedPhases, maxElementUB );

    // f_UB and maxElementUB need to be equal. If not, the lower of the two wins.
    if ( FloatUtils::areDisequal( fUB, maxElementUB ) )
    {
        if ( FloatUtils::gt( fUB, maxElementUB ) )
            tightenings.append( Tightening( _f, maxElementUB, Tightening::UB ) );
        else
            // f_UB <= maxElementUB
            for ( const auto &element : _elements )
                if ( !existsUpperBound( element ) ||
                     FloatUtils::gt( getUpperBound( element ), fUB ) )
                    tightenings.append( Tightening( element, fUB, Tightening::UB ) );
    }

    // fLB cannot be smaller than maxElementLB
    if ( FloatUtils::lt( fLB, maxElementLB ) )
        tightenings.append( Tightening( _f, maxElementLB, Tightening::LB ) );

    // TODO: bound tightening for aux vars.
}

bool MaxConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _f ) || _elements.exists( variable ) || _auxToElement.exists( variable );
}

List<unsigned> MaxConstraint::getParticipatingVariables() const
{
    List<unsigned> result;

    for ( auto element : _elements )
        result.append( element );

    for ( auto pair : _auxToElement )
        result.append( pair.first );

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
    DEBUG( {
        if ( !( existsAssignment( _f ) ) )
            throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT,
                                Stringf( "f(x%u) assignment missing.", _f ).ascii() );
        for ( const auto &element : _elements )
            if ( !( existsAssignment( element ) ) )
                throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT,
                                    Stringf( "input(x%u) assignment missing.", element ).ascii() );
    } );

    double fValue = getAssignment( _f );
    double maxValue = _maxValueOfEliminatedPhases;
    for ( const auto &element : _elements )
    {
        double currentValue = getAssignment( element );
        if ( FloatUtils::gt( currentValue, maxValue ) )
            maxValue = currentValue;
    }
    return FloatUtils::areEqual( maxValue, fValue );
}

bool MaxConstraint::isCaseInfeasible( unsigned variable ) const
{
    return PiecewiseLinearConstraint::isCaseInfeasible( variableToPhase( variable ) );
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getPossibleFixes() const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );
    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearConstraint::Fix> MaxConstraint::getSmartFixes( ITableau * ) const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );
    return getPossibleFixes();
}

List<PhaseStatus> MaxConstraint::getAllCases() const
{
    List<PhaseStatus> cases;
    for ( unsigned element : _elements )
        cases.append( variableToPhase( element ) );

    if ( _haveFeasibleEliminatedPhases )
        cases.append( MAX_PHASE_ELIMINATED );

    return cases;
}

List<PiecewiseLinearCaseSplit> MaxConstraint::getCaseSplits() const
{
    List<PiecewiseLinearCaseSplit> splits;

    for ( const auto &phase : getAllCases() )
        splits.append( getCaseSplit( phase ) );

    return splits;
}

bool MaxConstraint::phaseFixed() const
{
    return ( ( _elements.size() == 1 && !_haveFeasibleEliminatedPhases ) ||
             ( _elements.size() == 0 && _haveFeasibleEliminatedPhases ) );
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

    return getCaseSplit( phase );
}

PiecewiseLinearCaseSplit MaxConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

PiecewiseLinearCaseSplit MaxConstraint::getCaseSplit( PhaseStatus phase ) const
{
    if ( phase == MAX_PHASE_ELIMINATED )
    {
        PiecewiseLinearCaseSplit eliminatedPhase;
        eliminatedPhase.storeBoundTightening(
            Tightening( _f, _maxValueOfEliminatedPhases, Tightening::LB ) );
        eliminatedPhase.storeBoundTightening(
            Tightening( _f, _maxValueOfEliminatedPhases, Tightening::UB ) );
        return eliminatedPhase;
    }
    else
    {
        unsigned argMax = phaseToVariable( phase );
        PiecewiseLinearCaseSplit maxPhase;

        if ( argMax != _f )
        {
            // We had f - argMax = aux and
            maxPhase.storeBoundTightening( Tightening( _elementToAux[argMax], 0, Tightening::UB ) );
        }
        return maxPhase;
    }
}

void MaxConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable re-indexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

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

        if ( _phaseStatus == variableToPhase( oldIndex ) )
            _phaseStatus = variableToPhase( newIndex );
    }
    else
    {
        ASSERT( _auxToElement.exists( oldIndex ) );
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
        eliminateCase( var );

        _maxLowerBound = FloatUtils::max( value, _maxLowerBound );

        _maxValueOfEliminatedPhases = FloatUtils::max( value, _maxValueOfEliminatedPhases );

        _haveFeasibleEliminatedPhases =
            FloatUtils::gte( _maxValueOfEliminatedPhases, _maxLowerBound );
    }
    else if ( _auxToElement.exists( var ) )
    {
        // Aux var
        unsigned currentElement = _auxToElement[var];
        if ( FloatUtils::isZero( value ) )
        {
            _obsolete = true;
        }
        else
        {
            // The case corresponding to aux is infeasible.
            eliminateCase( currentElement );
        }
    }

    if ( phaseFixed() )
        _phaseStatus = ( _haveFeasibleEliminatedPhases ? MAX_PHASE_ELIMINATED
                                                       : variableToPhase( *_elements.begin() ) );

    if ( _elements.size() == 0 )
        _obsolete = true;
}

void MaxConstraint::transformToUseAuxVariables( Query &inputQuery )
{
    if ( _auxToElement.size() > 0 )
        return;

    bool fInInput = false;
    for ( const auto &element : _elements )
    {
        // If element is equal to _f, skip this step.
        // The reason is to avoid adding equations like `1.00x00 -1.00x00 -1.00x01 = 0.00`.
        if ( element == _f )
        {
            fInInput = true;
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
    if ( fInInput )
    {
        List<unsigned> toRemove;
        for ( const auto &element : _elements )
        {
            if ( element == _f )
                continue;
            toRemove.append( element );
        }
        for ( const auto &element : toRemove )
            eliminateCase( element );
        _obsolete = true;
    }
}

void MaxConstraint::getCostFunctionComponent( LinearExpression &cost, PhaseStatus phase ) const
{
    // If the constraint is not active or is fixed, it contributes nothing
    if ( !isActive() || phaseFixed() )
        return;

    if ( phase == MAX_PHASE_ELIMINATED )
    {
        // The cost term corresponding to this phase is f - maxValueOfEliminated.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        cost._addends[_f] = cost._addends[_f] + 1;
        cost._constant -= _maxValueOfEliminatedPhases;
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

PhaseStatus
MaxConstraint::getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const
{
    auto byAssignment = [&]( const unsigned &a, const unsigned &b ) {
        return assignment[a] < assignment[b];
    };
    unsigned largestVariable =
        *std::max_element( _elements.begin(), _elements.end(), byAssignment );
    double value = assignment[largestVariable];
    if ( _haveFeasibleEliminatedPhases && FloatUtils::lt( value, _maxValueOfEliminatedPhases ) )
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
    output += Stringf( ",%u", _haveFeasibleEliminatedPhases ? 1 : 0 );
    if ( _haveFeasibleEliminatedPhases )
        output += Stringf( ",%f", _maxValueOfEliminatedPhases );
    else
        // Will be ignored in any case
        output += Stringf( ",%u", 0 );

    return output;
}

void MaxConstraint::eliminateCase( unsigned variable )
{
    bool proofs = _boundManager && _boundManager->shouldProduceProofs();

    // Function is not yet supported for proof production
    if ( proofs )
        return;

    if ( _cdInfeasibleCases )
    {
        markInfeasible( variableToPhase( variable ) );
    }
    else
    {
        _elements.erase( variable );
        _eliminatedElements.insert( variable );

        if ( _elementToAux.exists( variable ) )
        {
            unsigned aux = _elementToAux[variable];
            _elementToAux.erase( variable );
            _auxToElement.erase( aux );
        }
        if ( proofs )
        {
            if ( _elementToTighteningRow.exists( variable ) &&
                 _elementToTighteningRow[variable] != NULL )
            {
                _elementToTighteningRow[variable] = NULL;
                _elementToTighteningRow.erase( variable );
            }

            if ( _elementToTableauAux.exists( variable ) )
                _elementToTableauAux.erase( variable );
        }
    }
}

bool MaxConstraint::haveOutOfBoundVariables() const
{
    double fValue = getAssignment( _f );
    if ( FloatUtils::gt( getLowerBound( _f ), fValue ) ||
         FloatUtils::lt( getUpperBound( _f ), fValue ) )
        return true;

    for ( const auto &element : _elements )
    {
        double value = getAssignment( element );
        if ( FloatUtils::gt( getLowerBound( element ), value ) ||
             FloatUtils::lt( getUpperBound( element ), value ) )
            return true;
        unsigned aux = _elementToAux[element];
        double auxValue = getAssignment( aux );
        if ( FloatUtils::gt( getLowerBound( aux ), auxValue ) ||
             FloatUtils::lt( getUpperBound( aux ), auxValue ) )
            return true;
    }
    return false;
}

void MaxConstraint::createElementTighteningRow( unsigned element )
{
    // Create the row only when needed and when not already created
    if ( !_boundManager->getBoundExplainer() || _elementToTighteningRow[element] != NULL )
        return;

    _elementToTighteningRow[element] = std::make_shared<TableauRow>( 3 );

    // f = element + aux + counterpart (an additional aux variable of tableau)
    _elementToTighteningRow[element]->_lhs = _f;
    _elementToTighteningRow[element]->_row[0] = TableauRow::Entry( element, 1 );
    _elementToTighteningRow[element]->_row[1] = TableauRow::Entry( _elementToAux[element], 1 );
    _elementToTighteningRow[element]->_row[2] =
        TableauRow::Entry( _elementToTableauAux[element], 1 );
    _elementToTighteningRow[element]->_scalar = 0;
}

const List<unsigned> MaxConstraint::getNativeAuxVars() const
{
    List<unsigned> auxVars = {};
    for ( const auto &element : _elements )
        auxVars.append( _elementToAux[element] );

    return auxVars;
}

void MaxConstraint::addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar )
{
    unsigned element = _auxToElement[constraintAuxVar];
    _elementToTableauAux[element] = tableauAuxVar;
    _elementToTighteningRow[element] = nullptr;
}

void MaxConstraint::applyTightenings( const List<Tightening> &tightenings ) const
{
    bool proofs = _boundManager && _boundManager->shouldProduceProofs();

    for ( const auto &tightening : tightenings )
    {
        if ( tightening._type == Tightening::LB )
        {
            if ( proofs )
            {
                unsigned maxElementForLB = _f;
                double maxElementLB = FloatUtils::negativeInfinity();

                for ( const auto &element : _elements )
                {
                    if ( existsLowerBound( element ) )
                    {
                        maxElementLB = FloatUtils::max( getLowerBound( element ), maxElementLB );
                        if ( maxElementLB == getLowerBound( element ) )
                            maxElementForLB = element;
                    }
                }

                ASSERT( _elements.exists( maxElementForLB ) &&
                        _elementToTighteningRow[maxElementForLB] != NULL );
                ASSERT( tightening._variable == _f );
                _boundManager->tightenLowerBound(
                    _f, maxElementLB, *_elementToTighteningRow[maxElementForLB] );
            }
            else
                _boundManager->tightenLowerBound( tightening._variable, tightening._value );
        }
        else if ( tightening._type == Tightening::UB )
        {
            if ( proofs )
            {
                if ( tightening._variable == _f )
                    _boundManager->addLemmaExplanationAndTightenBound( _f,
                                                                       tightening._value,
                                                                       Tightening::UB,
                                                                       getElements(),
                                                                       Tightening::UB,
                                                                       getType() );
                else
                {
                    ASSERT( _elements.exists( tightening._variable ) );
                    ASSERT( _elementToTighteningRow[tightening._variable] != NULL );
                    _boundManager->tightenUpperBound(
                        tightening._variable,
                        tightening._value,
                        *_elementToTighteningRow[tightening._variable] );
                }
            }
            else
                _boundManager->tightenUpperBound( tightening._variable, tightening._value );
        }
    }
}
