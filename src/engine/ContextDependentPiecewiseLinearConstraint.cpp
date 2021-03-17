/*********************                                                        */
/*! \file ContextDependentPiecewiseLinearConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A context dependent implementation of ContextDependentPiecewiseLinearConstraint class
 **/

#include "ContextDependentPiecewiseLinearConstraint.h"

#include "Statistics.h"

ContextDependentPiecewiseLinearConstraint::ContextDependentPiecewiseLinearConstraint()
   : _numCases( 0 )
   , _boundManager( nullptr )
   , _context( nullptr )
   , _cdConstraintActive( nullptr )
   , _cdPhaseStatus( nullptr )
   , _cdInfeasibleCases( nullptr )
{
}

ContextDependentPiecewiseLinearConstraint::ContextDependentPiecewiseLinearConstraint(
    unsigned numCases )
   : _numCases( numCases )
   , _boundManager( nullptr )
   , _context( nullptr )
   , _cdConstraintActive( nullptr )
   , _cdPhaseStatus( nullptr )
   , _cdInfeasibleCases( nullptr )
{
}

void ContextDependentPiecewiseLinearConstraint::setActiveConstraint( bool active )
{
    if ( _cdConstraintActive != nullptr )
        *_cdConstraintActive = active;
    else
        _constraintActive = active;
}

bool ContextDependentPiecewiseLinearConstraint::isActive() const
{
    if ( _cdConstraintActive != nullptr )
        return *_cdConstraintActive;
    else
        return _constraintActive;
}

void ContextDependentPiecewiseLinearConstraint::registerBoundManager(
    BoundManager *boundManager )
{
    ASSERT( _boundManager == nullptr );
    _boundManager = boundManager;
}

void ContextDependentPiecewiseLinearConstraint::initializeCDOs(
    CVC4::context::Context *context )
{
    ASSERT( _context == nullptr );
    _context = context;

    initializeCDActiveStatus();
    initializeCDPhaseStatus();
    initializeCDInfeasibleCases();
}

void ContextDependentPiecewiseLinearConstraint::initializeCDInfeasibleCases()
{
    ASSERT( _context != nullptr );
    ASSERT( _cdInfeasibleCases == nullptr );
    _cdInfeasibleCases = new ( true ) CVC4::context::CDList<PhaseStatus>( _context );
}

void ContextDependentPiecewiseLinearConstraint::initializeCDActiveStatus()
{
    ASSERT( _context != nullptr );
    ASSERT( _cdConstraintActive == nullptr );
    _cdConstraintActive = new ( true ) CVC4::context::CDO<bool>( _context, _constraintActive );
}

void ContextDependentPiecewiseLinearConstraint::initializeCDPhaseStatus()
{
    ASSERT( _context != nullptr );
    ASSERT( _cdPhaseStatus == nullptr );
    _cdPhaseStatus =
        new ( true ) CVC4::context::CDO<PhaseStatus>( _context, _phaseStatus );
}

void ContextDependentPiecewiseLinearConstraint::cdoCleanup()
{
    if ( _cdConstraintActive != nullptr )
        _cdConstraintActive->deleteSelf();

    _cdConstraintActive = nullptr;

    if ( _cdPhaseStatus != nullptr )
        _cdPhaseStatus->deleteSelf();

    _cdPhaseStatus = nullptr;

    if ( _cdInfeasibleCases != nullptr )
        _cdInfeasibleCases->deleteSelf();

    _cdInfeasibleCases = nullptr;

    _context = nullptr;
}

PhaseStatus ContextDependentPiecewiseLinearConstraint::getPhaseStatus() const
{
    if ( _cdPhaseStatus != nullptr )
        return *_cdPhaseStatus;
    else
        return _phaseStatus;
}

void ContextDependentPiecewiseLinearConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    if ( _cdPhaseStatus != nullptr )
        *_cdPhaseStatus = phaseStatus;
    else
        _phaseStatus = phaseStatus;
}

void ContextDependentPiecewiseLinearConstraint::initializeDuplicatesCDOs(
    ContextDependentPiecewiseLinearConstraint *clone ) const
{
    if ( clone->_context != nullptr )
    {
        ASSERT( clone->_cdConstraintActive != nullptr );
        clone->_cdConstraintActive = nullptr;
        clone->initializeCDActiveStatus();
        clone->setActiveConstraint( this->isActive() );

        ASSERT( clone->_cdPhaseStatus != nullptr );
        clone->_cdPhaseStatus = nullptr;
        clone->initializeCDPhaseStatus();
        clone->setPhaseStatus( this->getPhaseStatus() );

        ASSERT( clone->_cdInfeasibleCases != nullptr );
        clone->_cdInfeasibleCases = nullptr;
        clone->initializeCDInfeasibleCases();
        // Does not copy contents
    }
}

void ContextDependentPiecewiseLinearConstraint::markInfeasible(
    PhaseStatus infeasibleCase )
{
    _cdInfeasibleCases->push_back( infeasibleCase );
}

PhaseStatus ContextDependentPiecewiseLinearConstraint::nextFeasibleCase()
{
    ASSERT( getPhaseStatus() == PHASE_NOT_FIXED );

    if ( !isFeasible() )
        return CONSTRAINT_INFEASIBLE;

    List<PhaseStatus> allCases = getAllCases();
    for ( PhaseStatus thisCase : allCases )
    {
        auto loc =
            std::find( _cdInfeasibleCases->begin(), _cdInfeasibleCases->end(), thisCase );

        // Case is not infeasible, return it as feasible
        if ( loc == _cdInfeasibleCases->end() )
            return thisCase;
    }

    // UNREACHABLE
    ASSERT( false );
    return CONSTRAINT_INFEASIBLE;
}

bool ContextDependentPiecewiseLinearConstraint::isCaseInfeasible( PhaseStatus phase ) const
{
    ASSERT( _cdInfeasibleCases );
    return std::find( _cdInfeasibleCases->begin(), _cdInfeasibleCases->end(), phase ) != _cdInfeasibleCases->end();
}

