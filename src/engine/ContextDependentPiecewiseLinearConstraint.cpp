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
    if ( nullptr != _cdConstraintActive )
        *_cdConstraintActive = active;
    else
        _constraintActive = active;
}

bool ContextDependentPiecewiseLinearConstraint::isActive() const
{
    if ( nullptr != _cdConstraintActive )
        return *_cdConstraintActive;
    else
        return _constraintActive;
}

void ContextDependentPiecewiseLinearConstraint::registerBoundManager(
    BoundManager *boundManager )
{
    ASSERT( nullptr == _boundManager );
    _boundManager = boundManager;
}

void ContextDependentPiecewiseLinearConstraint::initializeCDOs(
    CVC4::context::Context *context )
{
    ASSERT( nullptr == _context );
    _context = context;

    initializeCDActiveStatus();
    initializeCDPhaseStatus();
    initializeCDInfeasibleCases();
}

void ContextDependentPiecewiseLinearConstraint::initializeCDInfeasibleCases()
{
    ASSERT( nullptr != _context );
    ASSERT( nullptr == _cdInfeasibleCases );
    _cdInfeasibleCases = new ( true ) CVC4::context::CDList<PhaseStatus>( _context );
}

void ContextDependentPiecewiseLinearConstraint::initializeCDActiveStatus()
{
    ASSERT( nullptr != _context );
    ASSERT( nullptr == _cdConstraintActive );
    _cdConstraintActive = new ( true ) CVC4::context::CDO<bool>( _context, true );
}

void ContextDependentPiecewiseLinearConstraint::initializeCDPhaseStatus()
{
    ASSERT( nullptr != _context );
    ASSERT( nullptr == _cdPhaseStatus );
    _cdPhaseStatus =
        new ( true ) CVC4::context::CDO<PhaseStatus>( _context, PHASE_NOT_FIXED );
}

void ContextDependentPiecewiseLinearConstraint::cdoCleanup()
{
    if ( nullptr != _cdConstraintActive )
        _cdConstraintActive->deleteSelf();

    _cdConstraintActive = nullptr;

    if ( nullptr != _cdPhaseStatus )
        _cdPhaseStatus->deleteSelf();

    _cdPhaseStatus = nullptr;

    if ( nullptr != _cdInfeasibleCases )
        _cdInfeasibleCases->deleteSelf();

    _cdInfeasibleCases = nullptr;

    _context = nullptr;
}

PhaseStatus ContextDependentPiecewiseLinearConstraint::getPhaseStatus() const
{
    if ( nullptr != _cdPhaseStatus )
        return *_cdPhaseStatus;
    else
        return _phaseStatus;
}

void ContextDependentPiecewiseLinearConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    if ( nullptr != _cdPhaseStatus )
        *_cdPhaseStatus = phaseStatus;
    else
        _phaseStatus = phaseStatus;
}

void ContextDependentPiecewiseLinearConstraint::initializeDuplicatesCDOs(
    ContextDependentPiecewiseLinearConstraint *clone ) const
{
    if ( nullptr != clone->_context )
    {
        ASSERT( nullptr != clone->_cdConstraintActive );
        clone->_cdConstraintActive = nullptr;
        clone->initializeCDActiveStatus();
        clone->setActiveConstraint( this->isActive() );

        ASSERT( nullptr != clone->_cdPhaseStatus );
        clone->_cdPhaseStatus = nullptr;
        clone->initializeCDPhaseStatus();
        clone->setPhaseStatus( this->getPhaseStatus() );

        ASSERT( nullptr != clone->_cdInfeasibleCases );
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
    if ( !isFeasible() )
        return PHASE_NOT_FIXED;

    if ( phaseFixed() )
        return getPhaseStatus();

    List<PhaseStatus> allCases = getAllCases();
    for ( PhaseStatus thisCase : allCases )
    {
        auto loc =
            std::find( _cdInfeasibleCases->begin(), _cdInfeasibleCases->end(), thisCase );

        // Case not found, therefore it is feasible
        if ( _cdInfeasibleCases->end() == loc )
            return thisCase;
    }

    // UNREACHABLE
    ASSERT( false );
    return PHASE_NOT_FIXED;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
