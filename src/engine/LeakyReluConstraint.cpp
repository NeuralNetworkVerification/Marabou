/*********************                                                        */
/*! \file LeakyReluConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "LeakyReluConstraint.h"

#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "Query.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__( x )
#endif

LeakyReluConstraint::LeakyReluConstraint( unsigned b, unsigned f, double slope )
    : PiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _slope( slope )
    , _auxVarsInUse( false )
    , _activeTighteningRow( NULL )
    , _inactiveTighteningRow( NULL )
    , _direction( PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
    if ( _slope <= 0 )
        throw MarabouError( MarabouError::INVALID_LEAKY_RELU_SLOPE );
}

LeakyReluConstraint::LeakyReluConstraint( const String &serializedLeakyRelu )
    : _activeTighteningRow( NULL )
    , _inactiveTighteningRow( NULL )
    , _haveEliminatedVariables( false )
{
    String constraintType = serializedLeakyRelu.substring( 0, 10 );
    ASSERT( constraintType == String( "leaky_relu" ) );

    // Remove the constraint type in serialized form
    String serializedValues =
        serializedLeakyRelu.substring( 11, serializedLeakyRelu.length() - 11 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 3 || values.size() == 5 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
    ++var;
    _slope = atof( var->ascii() );
    if ( _slope <= 0 || _slope > 1 )
        throw MarabouError( MarabouError::INVALID_LEAKY_RELU_SLOPE,
                            Stringf( "Currently supporting slope between 0 and 1" ).ascii() );

    _direction = PHASE_NOT_FIXED;

    if ( values.size() == 5 )
    {
        ++var;
        _activeAux = atoi( var->ascii() );
        ++var;
        _inactiveAux = atoi( var->ascii() );
        _auxVarsInUse = true;
    }
    else
        _auxVarsInUse = false;
}

PiecewiseLinearFunctionType LeakyReluConstraint::getType() const
{
    return PiecewiseLinearFunctionType::LEAKY_RELU;
}

PiecewiseLinearConstraint *LeakyReluConstraint::duplicateConstraint() const
{
    LeakyReluConstraint *clone = new LeakyReluConstraint( _b, _f, _slope );
    *clone = *this;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void LeakyReluConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const LeakyReluConstraint *leakyRelu = dynamic_cast<const LeakyReluConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *leakyRelu;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void LeakyReluConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );

    if ( _auxVarsInUse )
    {
        tableau->registerToWatchVariable( this, _activeAux );
        tableau->registerToWatchVariable( this, _inactiveAux );
    }
}

void LeakyReluConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );

    if ( _auxVarsInUse )
    {
        tableau->unregisterToWatchVariable( this, _activeAux );
        tableau->unregisterToWatchVariable( this, _inactiveAux );
    }
}

void LeakyReluConstraint::checkIfLowerBoundUpdateFixesPhase( unsigned variable, double bound )
{
    if ( variable == _f && !FloatUtils::isNegative( bound ) )
        setPhaseStatus( RELU_PHASE_ACTIVE );
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
        setPhaseStatus( RELU_PHASE_ACTIVE );
    else if ( _auxVarsInUse )
    {
        if ( variable == _activeAux && FloatUtils::isPositive( bound ) )
            setPhaseStatus( RELU_PHASE_INACTIVE );
        else if ( variable == _inactiveAux && FloatUtils::isPositive( bound ) )
            setPhaseStatus( RELU_PHASE_ACTIVE );
    }
}

void LeakyReluConstraint::checkIfUpperBoundUpdateFixesPhase( unsigned variable, double bound )
{
    if ( variable == _f && FloatUtils::isNegative( bound ) )
        setPhaseStatus( RELU_PHASE_INACTIVE );
    else if ( variable == _b && FloatUtils::isNegative( bound ) )
        setPhaseStatus( RELU_PHASE_INACTIVE );
    else if ( _auxVarsInUse )
    {
        if ( variable == _activeAux && FloatUtils::isZero( bound ) )
            setPhaseStatus( RELU_PHASE_ACTIVE );
        else if ( variable == _inactiveAux && FloatUtils::isZero( bound ) )
            setPhaseStatus( RELU_PHASE_INACTIVE );
    }
}

void LeakyReluConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsLowerBound( variable ) &&
         !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;
    setLowerBound( variable, bound );

    if ( isActive() && _boundManager && !phaseFixed() )
    {
        bool proofs = _boundManager->shouldProduceProofs();

        if ( proofs )
        {
            createActiveTighteningRow();
            createInactiveTighteningRow();
        }

        // A positive lower bound is always propagated between f and b
        if ( variable == _f || variable == _b )
        {
            if ( FloatUtils::gte( bound, 0 ) )
            {
                // If we're in the active phase, activeAux should be 0
                if ( proofs )
                    _boundManager->addLemmaExplanationAndTightenBound(
                        _activeAux, 0, Tightening::UB, { variable }, Tightening::LB, getType() );
                else if ( !proofs && _auxVarsInUse )
                    _boundManager->tightenUpperBound( _activeAux, 0 );

                // After updating to active phase
                unsigned partner = ( variable == _f ) ? _b : _f;
                _boundManager->tightenLowerBound( partner, bound, *_activeTighteningRow );
            }
            else if ( variable == _b && FloatUtils::isNegative( bound ) )
            {
                _boundManager->tightenLowerBound( _f, _slope * bound, *_inactiveTighteningRow );
            }
            else if ( variable == _f && FloatUtils::isNegative( bound ) )
            {
                if ( proofs )
                    _boundManager->addLemmaExplanationAndTightenBound(
                        _b, bound / _slope, Tightening::LB, { _f }, Tightening::LB, getType() );
                else
                    _boundManager->tightenLowerBound( _b, bound / _slope, *_inactiveTighteningRow );
            }
        }

        // A positive lower bound for activeAux means we're inactive: _inactiveAux <= 0
        else if ( _auxVarsInUse && variable == _activeAux && bound > 0 )
        {
            // Inactive phase
            if ( proofs )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _inactiveAux, 0, Tightening::UB, { _activeAux }, Tightening::LB, getType() );
            else
                _boundManager->tightenUpperBound( _inactiveAux, 0 );
        }

        // A positive lower bound for inactiveAux means we're active: _activeAux <= 0
        else if ( _auxVarsInUse && variable == _inactiveAux && bound > 0 )
        {
            // Active phase
            if ( proofs )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _activeAux, 0, Tightening::UB, { _inactiveAux }, Tightening::LB, getType() );
            else
                _boundManager->tightenUpperBound( _activeAux, 0 );
        }
    }

    checkIfLowerBoundUpdateFixesPhase( variable, bound );
}

void LeakyReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsUpperBound( variable ) &&
         !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    setUpperBound( variable, bound );

    if ( isActive() && _boundManager && !phaseFixed() )
    {
        bool proofs = _boundManager->shouldProduceProofs();

        if ( proofs )
        {
            createActiveTighteningRow();
            createInactiveTighteningRow();
        }

        // A positive upper bound is always propagated between f and b
        if ( variable == _f || variable == _b )
        {
            if ( !FloatUtils::isNegative( bound ) )
            {
                unsigned partner = ( variable == _f ) ? _b : _f;
                if ( proofs )
                    _boundManager->addLemmaExplanationAndTightenBound(
                        partner, bound, Tightening::UB, { variable }, Tightening::UB, getType() );
                else
                    _boundManager->tightenUpperBound( partner, bound );
            }
            else if ( variable == _b )
            {
                // A negative upper bound of b implies inactive phase
                if ( proofs && _auxVarsInUse )
                    _boundManager->addLemmaExplanationAndTightenBound(
                        _inactiveAux, 0, Tightening::UB, { _b }, Tightening::UB, getType() );

                _boundManager->tightenUpperBound( _f, _slope * bound, *_inactiveTighteningRow );
            }
            else if ( variable == _f )
            {
                // A negative upper bound of f implies inactive phase as well
                if ( proofs && _auxVarsInUse )
                    _boundManager->addLemmaExplanationAndTightenBound(
                        _inactiveAux, 0, Tightening::UB, { _f }, Tightening::UB, getType() );

                _boundManager->tightenUpperBound( _b, bound / _slope, *_inactiveTighteningRow );
            }
        }
    }

    checkIfUpperBoundUpdateFixesPhase( variable, bound );
}

bool LeakyReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f ) ||
           ( _auxVarsInUse && ( variable == _activeAux || variable == _inactiveAux ) );
}

List<unsigned> LeakyReluConstraint::getParticipatingVariables() const
{
    return _auxVarsInUse ? List<unsigned>( { _b, _f, _activeAux, _inactiveAux } )
                         : List<unsigned>( { _b, _f } );
}

bool LeakyReluConstraint::satisfied() const
{
    if ( !( existsAssignment( _b ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual(
            bValue, fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
    else
        return FloatUtils::areEqual(
            _slope * bValue, fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
}

List<PiecewiseLinearConstraint::Fix> LeakyReluConstraint::getPossibleFixes() const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );

    ASSERT( !satisfied() );
    ASSERT( existsAssignment( _b ) );
    ASSERT( existsAssignment( _f ) );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   Case 1. f is positive, b is positive, b and f are disequal
    //   Case 2. f is positive, b is non-positive
    //   Case 3. f is non-positive, b is non-positive, f is not equal to slope * b
    //   Case 4. f is non-positive, b is positive
    if ( FloatUtils::isPositive( fValue ) )
    {
        if ( FloatUtils::isPositive( bValue ) )
        {
            // Case 1
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
        }
        else
        {
            // Case 2
            if ( _direction == RELU_PHASE_INACTIVE )
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _f, _slope * bValue ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            }
            else
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _f, _slope * bValue ) );
            }
        }
    }
    else
    {
        if ( !FloatUtils::isPositive( bValue ) )
        {
            // Case 3
            fixes.append( PiecewiseLinearConstraint::Fix( _f, _slope * bValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue / _slope ) );
        }
        else
        {
            // Case 4
            if ( _direction == RELU_PHASE_ACTIVE )
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue / _slope ) );
            }
            else
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue / _slope ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
            }
        }
    }

    return fixes;
}

List<PiecewiseLinearConstraint::Fix> LeakyReluConstraint::getSmartFixes( ITableau * ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> LeakyReluConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;

    if ( _direction == RELU_PHASE_INACTIVE )
    {
        splits.append( getInactiveSplit() );
        splits.append( getActiveSplit() );
        return splits;
    }
    if ( _direction == RELU_PHASE_ACTIVE )
    {
        splits.append( getActiveSplit() );
        splits.append( getInactiveSplit() );
        return splits;
    }

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( existsAssignment( _f ) )
    {
        if ( FloatUtils::isPositive( getAssignment( _f ) ) )
        {
            splits.append( getActiveSplit() );
            splits.append( getInactiveSplit() );
        }
        else
        {
            splits.append( getInactiveSplit() );
            splits.append( getActiveSplit() );
        }
    }
    else
    {
        splits.append( getInactiveSplit() );
        splits.append( getActiveSplit() );
    }

    return splits;
}

List<PhaseStatus> LeakyReluConstraint::getAllCases() const
{
    if ( _direction == RELU_PHASE_INACTIVE )
        return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };

    if ( _direction == RELU_PHASE_ACTIVE )
        return { RELU_PHASE_ACTIVE, RELU_PHASE_INACTIVE };

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( existsAssignment( _f ) )
    {
        if ( FloatUtils::isPositive( getAssignment( _f ) ) )
            return { RELU_PHASE_ACTIVE, RELU_PHASE_INACTIVE };
        else
            return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };
    }
    else
        return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getCaseSplit( PhaseStatus phase ) const
{
    if ( phase == RELU_PHASE_INACTIVE )
        return getInactiveSplit();
    else if ( phase == RELU_PHASE_ACTIVE )
        return getActiveSplit();
    else
        throw MarabouError( MarabouError::REQUESTED_NONEXISTENT_CASE_SPLIT );
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getInactiveSplit() const
{
    // Inactive phase: b <= 0, f <= 0, f = slope * b
    PiecewiseLinearCaseSplit inactivePhase;
    inactivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    inactivePhase.storeBoundTightening( Tightening( _f, 0.0, Tightening::UB ) );

    if ( _auxVarsInUse )
    {
        // We added inactiveAux = f - slope * b and inactiveAux >= 0
        inactivePhase.storeBoundTightening( Tightening( _inactiveAux, 0.0, Tightening::UB ) );
    }
    else
    {
        Equation inactiveEquation( Equation::EQ );
        inactiveEquation.addAddend( _slope, _b );
        inactiveEquation.addAddend( -1, _f );
        inactiveEquation.setScalar( 0 );
        inactivePhase.addEquation( inactiveEquation );
    }
    return inactivePhase;
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getActiveSplit() const
{
    // Active phase: b >= 0, f = b
    PiecewiseLinearCaseSplit activePhase;
    activePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );
    activePhase.storeBoundTightening( Tightening( _f, 0.0, Tightening::LB ) );

    if ( _auxVarsInUse )
    {
        // We added inactiveAux = f - b and auxActive >= 0
        activePhase.storeBoundTightening( Tightening( _activeAux, 0.0, Tightening::UB ) );
    }
    else
    {
        Equation activeEquation( Equation::EQ );
        activeEquation.addAddend( 1, _b );
        activeEquation.addAddend( -1, _f );
        activeEquation.setScalar( 0 );
        activePhase.addEquation( activeEquation );
    }

    return activePhase;
}

bool LeakyReluConstraint::phaseFixed() const
{
    return _phaseStatus != PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getImpliedCaseSplit() const
{
    ASSERT( _phaseStatus != PHASE_NOT_FIXED );

    if ( _phaseStatus == RELU_PHASE_ACTIVE )
        return getActiveSplit();

    return getInactiveSplit();
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

void LeakyReluConstraint::dump( String &output ) const
{
    output = Stringf( "LeakyReluConstraint: x%u = LeakyReLU( x%u ), slope = %lf. Active? %s. "
                      "PhaseStatus = %u (%s).\n",
                      _f,
                      _b,
                      _slope,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus,
                      phaseToString( _phaseStatus ).ascii() );

    output +=
        Stringf( "b in [%s, %s], ",
                 existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                 existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output +=
        Stringf( "f in [%s, %s]",
                 existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "-inf",
                 existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "inf" );

    if ( _auxVarsInUse )
    {
        output += Stringf(
            ". Active aux var: %u. Range: [%s, %s]\n",
            _activeAux,
            existsLowerBound( _activeAux ) ? Stringf( "%lf", getLowerBound( _activeAux ) ).ascii()
                                           : "-inf",
            existsUpperBound( _activeAux ) ? Stringf( "%lf", getUpperBound( _activeAux ) ).ascii()
                                           : "inf" );
        output += Stringf( ". Inactive aux var: %u. Range: [%s, %s]\n",
                           _inactiveAux,
                           existsLowerBound( _inactiveAux )
                               ? Stringf( "%lf", getLowerBound( _inactiveAux ) ).ascii()
                               : "-inf",
                           existsUpperBound( _inactiveAux )
                               ? Stringf( "%lf", getUpperBound( _inactiveAux ) ).ascii()
                               : "inf" );
    }
}

void LeakyReluConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable reindexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

    ASSERT( participatingVariable( oldIndex ) );
    ASSERT( !_lowerBounds.exists( newIndex ) && !_upperBounds.exists( newIndex ) &&
            !participatingVariable( newIndex ) );

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

    if ( oldIndex == _b )
        _b = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
    else if ( oldIndex == _activeAux )
        _activeAux = newIndex;
    else
        _inactiveAux = newIndex;
}

void LeakyReluConstraint::eliminateVariable( __attribute__( ( unused ) ) unsigned variable,
                                             __attribute__( ( unused ) ) double fixedValue )
{
    ASSERT( participatingVariable( variable ) );
    DEBUG( {
        if ( variable == _f || variable == _b )
        {
            if ( FloatUtils::gt( fixedValue, 0 ) )
            {
                ASSERT( _phaseStatus != RELU_PHASE_INACTIVE );
            }
            else if ( FloatUtils::lt( fixedValue, 0 ) )
            {
                ASSERT( _phaseStatus != RELU_PHASE_ACTIVE );
            }
        }
        else if ( variable == _activeAux )
        {
            if ( FloatUtils::isPositive( fixedValue ) )
            {
                ASSERT( _phaseStatus != RELU_PHASE_ACTIVE );
            }
        }
        else
        {
            // This is the inactive aux variable
            if ( FloatUtils::isPositive( fixedValue ) )
            {
                ASSERT( _phaseStatus != RELU_PHASE_INACTIVE );
            }
        }
    } );

    // In a Leaky ReLU constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool LeakyReluConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void LeakyReluConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) && existsUpperBound( _b ) &&
            existsUpperBound( _f ) );

    ASSERT( !_auxVarsInUse ||
            ( existsLowerBound( _activeAux ) && existsUpperBound( _activeAux ) &&
              existsLowerBound( _inactiveAux ) && existsUpperBound( _inactiveAux ) ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );

    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    double activeAuxLowerBound = 0;
    double activeAuxUpperBound = 0;
    double inactiveAuxLowerBound = 0;
    double inactiveAuxUpperBound = 0;

    if ( _auxVarsInUse )
    {
        activeAuxLowerBound = getLowerBound( _activeAux );
        activeAuxUpperBound = getUpperBound( _activeAux );
        inactiveAuxLowerBound = getLowerBound( _inactiveAux );
        inactiveAuxUpperBound = getUpperBound( _inactiveAux );
    }

    // Determine if we are in the active phase, inactive phase or unknown phase
    if ( FloatUtils::isPositive( bLowerBound ) || FloatUtils::isPositive( fLowerBound ) ||
         ( _auxVarsInUse && ( FloatUtils::isZero( activeAuxUpperBound ) ||
                              FloatUtils::isPositive( inactiveAuxLowerBound ) ) ) )
    {
        // Active case;
        if ( FloatUtils::isPositive( fLowerBound ) )
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
        if ( FloatUtils::isPositive( bLowerBound ) )
            tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );

        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        // Aux is zero
        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _activeAux, 0, Tightening::LB ) );
            tightenings.append( Tightening( _activeAux, 0, Tightening::UB ) );
        }

        tightenings.append( Tightening( _b, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 0, Tightening::LB ) );
    }
    else if ( !FloatUtils::isPositive( bUpperBound ) || !FloatUtils::isPositive( fUpperBound ) ||
              ( _auxVarsInUse && ( FloatUtils::isZero( inactiveAuxUpperBound ) ||
                                   FloatUtils::isPositive( activeAuxLowerBound ) ) ) )
    {
        // Inactive case
        if ( FloatUtils::isFinite( fLowerBound ) )
            tightenings.append( Tightening( _b, fLowerBound / _slope, Tightening::LB ) );
        if ( FloatUtils::isFinite( bLowerBound ) )
            tightenings.append( Tightening( _f, _slope * bLowerBound, Tightening::LB ) );

        if ( FloatUtils::isFinite( fUpperBound ) && !FloatUtils::isPositive( fUpperBound ) )
            tightenings.append( Tightening( _b, fUpperBound / _slope, Tightening::UB ) );
        if ( FloatUtils::isFinite( bUpperBound ) && !FloatUtils::isPositive( bUpperBound ) )
            tightenings.append( Tightening( _f, _slope * bUpperBound, Tightening::UB ) );

        // Aux is zero
        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _inactiveAux, 0, Tightening::LB ) );
            tightenings.append( Tightening( _inactiveAux, 0, Tightening::UB ) );
        }

        tightenings.append( Tightening( _f, 0, Tightening::UB ) );
        tightenings.append( Tightening( _b, 0, Tightening::UB ) );
    }
    else
    {
        // Unknown case

        // b and f share upper bounds
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        if ( FloatUtils::isFinite( fLowerBound ) )
            tightenings.append( Tightening( _b, fLowerBound / _slope, Tightening::LB ) );
        if ( FloatUtils::isFinite( bLowerBound ) )
            tightenings.append( Tightening( _f, _slope * bLowerBound, Tightening::LB ) );
    }
}

String LeakyReluConstraint::phaseToString( PhaseStatus phase )
{
    switch ( phase )
    {
    case PHASE_NOT_FIXED:
        return "PHASE_NOT_FIXED";

    case RELU_PHASE_ACTIVE:
        return "RELU_PHASE_ACTIVE";

    case RELU_PHASE_INACTIVE:
        return "RELU_PHASE_INACTIVE";

    default:
        return "UNKNOWN";
    }
};

void LeakyReluConstraint::transformToUseAuxVariables( Query &inputQuery )
{
    /*
      We want to add the equations

          f >= b
          f >= slope * b

      Which actually becomes

          f - b - activeAux = 0
          f - slope * b - inactiveAux = 0

      Lower bounds: always non-negative
    */
    if ( _auxVarsInUse )
        return;

    // Create the aux variables
    _activeAux = inputQuery.getNumberOfVariables();
    _inactiveAux = _activeAux + 1;
    inputQuery.setNumberOfVariables( _activeAux + 2 );

    // Create and add the equations
    Equation equationActive( Equation::EQ );
    equationActive.addAddend( 1.0, _f );
    equationActive.addAddend( -1.0, _b );
    equationActive.addAddend( -1.0, _activeAux );
    equationActive.setScalar( 0 );
    inputQuery.addEquation( equationActive );

    Equation equationInactive( Equation::EQ );
    equationInactive.addAddend( 1.0, _f );
    equationInactive.addAddend( -_slope, _b );
    equationInactive.addAddend( -1.0, _inactiveAux );
    equationInactive.setScalar( 0 );
    inputQuery.addEquation( equationInactive );

    // Adjust the bounds for the new variables
    inputQuery.setLowerBound( _activeAux, 0 );
    inputQuery.setLowerBound( _inactiveAux, 0 );

    // We now care about the auxiliary variable, as well
    _auxVarsInUse = true;
}

void LeakyReluConstraint::getCostFunctionComponent( LinearExpression &cost,
                                                    PhaseStatus phase ) const
{
    // If the constraint is not active or is fixed, it contributes nothing
    if ( !isActive() || phaseFixed() )
        return;

    // This should not be called when the linear constraints have
    // not been satisfied
    ASSERT( !haveOutOfBoundVariables() );

    ASSERT( phase == RELU_PHASE_ACTIVE || phase == RELU_PHASE_INACTIVE );

    if ( phase == RELU_PHASE_INACTIVE )
    {
        // The cost term corresponding to the inactive phase is f - slope * b,
        // since the LeakyReLU is inactive and satisfied iff f is 0 and minimal.
        // This is true only when we added the constraint that f >= slope * b.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        if ( !cost._addends.exists( _b ) )
            cost._addends[_b] = 0;
        cost._addends[_f] = cost._addends[_f] + 1;
        cost._addends[_b] = cost._addends[_b] - _slope;
    }
    else
    {
        // The cost term corresponding to the inactive phase is f - b,
        // since the LeakyReLU is active and satisfied iff f - b is 0 and minimal.
        // Note that this is true only when we added the constraint that f >= b.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        if ( !cost._addends.exists( _b ) )
            cost._addends[_b] = 0;
        cost._addends[_f] = cost._addends[_f] + 1;
        cost._addends[_b] = cost._addends[_b] - 1;
    }
}

PhaseStatus
LeakyReluConstraint::getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const
{
    ASSERT( assignment.exists( _b ) );
    return FloatUtils::isNegative( assignment[_b] ) ? RELU_PHASE_INACTIVE : RELU_PHASE_ACTIVE;
}

bool LeakyReluConstraint::haveOutOfBoundVariables() const
{
    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    if ( FloatUtils::gt(
             getLowerBound( _b ), bValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) ||
         FloatUtils::lt(
             getUpperBound( _b ), bValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) )
        return true;

    if ( FloatUtils::gt(
             getLowerBound( _f ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) ||
         FloatUtils::lt(
             getUpperBound( _f ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) )
        return true;

    return false;
}

String LeakyReluConstraint::serializeToString() const
{
    // Output format is: relu,f,b,slope,activeAux,inactiveAux
    if ( _auxVarsInUse )
        return Stringf( "leaky_relu,%u,%u,%f,%u,%u", _f, _b, _slope, _activeAux, _inactiveAux );
    else
        return Stringf( "leaky_relu,%u,%u,%f", _f, _b, _slope );
}

unsigned LeakyReluConstraint::getB() const
{
    return _b;
}

unsigned LeakyReluConstraint::getF() const
{
    return _f;
}

double LeakyReluConstraint::getSlope() const
{
    return _slope;
}

bool LeakyReluConstraint::auxVariablesInUse() const
{
    return _auxVarsInUse;
}
unsigned LeakyReluConstraint::getActiveAux() const
{
    return _activeAux;
}
unsigned LeakyReluConstraint::getInactiveAux() const
{
    return _inactiveAux;
}

bool LeakyReluConstraint::supportPolarity() const
{
    return true;
}

double LeakyReluConstraint::computePolarity() const
{
    double currentLb = _lowerBounds[_b];
    double currentUb = _upperBounds[_b];
    if ( currentLb >= 0 )
        return 1;
    if ( currentUb <= 0 )
        return -1;
    double width = currentUb - currentLb;
    double sum = currentUb + currentLb;
    return sum / width;
}

void LeakyReluConstraint::updateDirection()
{
    _direction = ( computePolarity() > 0 ) ? RELU_PHASE_ACTIVE : RELU_PHASE_INACTIVE;
}

PhaseStatus LeakyReluConstraint::getDirection() const
{
    return _direction;
}

void LeakyReluConstraint::updateScoreBasedOnPolarity()
{
    _score = std::abs( computePolarity() );
}

const List<unsigned> LeakyReluConstraint::getNativeAuxVars() const
{
    if ( _auxVarsInUse )
        return { _activeAux, _inactiveAux };
    return {};
}

void LeakyReluConstraint::addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar )
{
    ASSERT( constraintAuxVar == _inactiveAux || constraintAuxVar == _activeAux );
    if ( _tableauAuxVars.size() == 2 )
        return;

    if ( constraintAuxVar == _inactiveAux )
    {
        _tableauAuxVars.append( tableauAuxVar );
        ASSERT( _tableauAuxVars.back() == tableauAuxVar );
    }
    else
    {
        _tableauAuxVars.appendHead( tableauAuxVar );
        ASSERT( _tableauAuxVars.front() == tableauAuxVar );
    }
}

void LeakyReluConstraint::createActiveTighteningRow()
{
    // Create the row only when needed and when not already created
    if ( !_boundManager->getBoundExplainer() || _activeTighteningRow || !_auxVarsInUse ||
         _tableauAuxVars.empty() )
        return;
    _activeTighteningRow = std::unique_ptr<TableauRow>( new TableauRow( 3 ) );

    // f = b + aux + counterpart (an additional aux variable of tableau)
    _activeTighteningRow->_lhs = _f;
    _activeTighteningRow->_row[0] = TableauRow::Entry( _b, 1 );
    _activeTighteningRow->_row[1] = TableauRow::Entry( _activeAux, 1 );
    _activeTighteningRow->_row[2] = TableauRow::Entry( _tableauAuxVars.front(), 1 );
    _activeTighteningRow->_scalar = 0;
}

void LeakyReluConstraint::createInactiveTighteningRow()
{
    // Create the row only when needed and when not already created
    if ( !_boundManager->getBoundExplainer() || _inactiveTighteningRow || !_auxVarsInUse ||
         _tableauAuxVars.empty() )
        return;

    _inactiveTighteningRow = std::unique_ptr<TableauRow>( new TableauRow( 3 ) );

    // f = b + aux + counterpart (an additional aux variable of tableau)
    _inactiveTighteningRow->_lhs = _f;
    _inactiveTighteningRow->_row[0] = TableauRow::Entry( _b, _slope );
    _inactiveTighteningRow->_row[1] = TableauRow::Entry( _inactiveAux, 1 );
    _inactiveTighteningRow->_row[2] = TableauRow::Entry( _tableauAuxVars.back(), 1 );
    _inactiveTighteningRow->_scalar = 0;
}
