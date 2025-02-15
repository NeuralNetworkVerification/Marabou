/*********************                                                        */
/*! \file SumOfInfeasibilitiesManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "SumOfInfeasibilitiesManager.h"

#include "FloatUtils.h"
#include "MarabouError.h"
#include "Options.h"
#include "Set.h"

SumOfInfeasibilitiesManager::SumOfInfeasibilitiesManager( const Query &inputQuery,
                                                          const ITableau &tableau )
    : _plConstraints( inputQuery.getPiecewiseLinearConstraints() )
    , _networkLevelReasoner( inputQuery.getNetworkLevelReasoner() )
    , _numberOfVariables( inputQuery.getNumberOfVariables() )
    , _tableau( tableau )
    , _initializationStrategy( Options::get()->getSoIInitializationStrategy() )
    , _searchStrategy( Options::get()->getSoISearchStrategy() )
    , _probabilityDensityParameter(
          Options::get()->getFloat( Options::PROBABILITY_DENSITY_PARAMETER ) )
    , _statistics( NULL )
{
    if ( !inputQuery._networkLevelReasoner ||
         inputQuery._networkLevelReasoner->getConstraintsInTopologicalOrder().size() <
             _plConstraints.size() )
        _initializationStrategy = SoIInitializationStrategy::CURRENT_ASSIGNMENT;
}

void SumOfInfeasibilitiesManager::resetPhasePattern()
{
    _currentPhasePattern.clear();
    _lastAcceptedPhasePattern.clear();
    _plConstraintsInCurrentPhasePattern.clear();
    _constraintsUpdatedInLastProposal.clear();
}

LinearExpression SumOfInfeasibilitiesManager::getCurrentSoIPhasePattern() const
{
    struct timespec start = TimeUtils::sampleMicro();

    LinearExpression cost;
    for ( const auto &pair : _currentPhasePattern )
        pair.first->getCostFunctionComponent( cost, pair.second );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
    return cost;
}

LinearExpression SumOfInfeasibilitiesManager::getLastAcceptedSoIPhasePattern() const
{
    struct timespec start = TimeUtils::sampleMicro();

    LinearExpression cost;
    for ( const auto &pair : _lastAcceptedPhasePattern )
        pair.first->getCostFunctionComponent( cost, pair.second );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
    return cost;
}

void SumOfInfeasibilitiesManager::initializePhasePattern()
{
    struct timespec start = TimeUtils::sampleMicro();

    resetPhasePattern();

    if ( _initializationStrategy == SoIInitializationStrategy::INPUT_ASSIGNMENT )
    {
        ASSERT( _networkLevelReasoner );
        initializePhasePatternWithCurrentInputAssignment();
    }
    else if ( _initializationStrategy == SoIInitializationStrategy::CURRENT_ASSIGNMENT )
    {
        initializePhasePatternWithCurrentAssignment();
    }
    else
    {
        throw MarabouError( MarabouError::UNABLE_TO_INITIALIZATION_PHASE_PATTERN );
    }

    // Store constraints participating in the SoI
    for ( const auto &pair : _currentPhasePattern )
        _plConstraintsInCurrentPhasePattern.append( pair.first );

    // The first phase pattern is always accepted.
    _lastAcceptedPhasePattern = _currentPhasePattern;

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SumOfInfeasibilitiesManager::initializePhasePatternWithCurrentInputAssignment()
{
    ASSERT( _networkLevelReasoner );
    /*
      First, obtain the variable assignment from the network level reasoner.
      We should be able to get the assignment of all variables participating
      in pl constraints because otherwise the NLR would not have been
      successfully constructed.
    */
    Map<unsigned, double> assignment;
    _networkLevelReasoner->concretizeInputAssignment( assignment );

    for ( const auto &plConstraint : _plConstraints )
    {
        ASSERT( !_currentPhasePattern.exists( plConstraint ) );
        if ( plConstraint->supportSoI() && plConstraint->isActive() && !plConstraint->phaseFixed() )
        {
            // Set the phase status corresponding to the current assignment.
            _currentPhasePattern[plConstraint] =
                plConstraint->getPhaseStatusInAssignment( assignment );
        }
    }
}

void SumOfInfeasibilitiesManager::initializePhasePatternWithCurrentAssignment()
{
    obtainCurrentAssignment();

    for ( const auto &plConstraint : _plConstraints )
    {
        ASSERT( !_currentPhasePattern.exists( plConstraint ) );
        if ( plConstraint->supportSoI() && plConstraint->isActive() && !plConstraint->phaseFixed() )
        {
            // Set the phase status corresponding to the current assignment.
            _currentPhasePattern[plConstraint] =
                plConstraint->getPhaseStatusInAssignment( _currentAssignment );
        }
    }
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdate()
{
    struct timespec start = TimeUtils::sampleMicro();

    _currentPhasePattern = _lastAcceptedPhasePattern;
    _constraintsUpdatedInLastProposal.clear();

    if ( _searchStrategy == SoISearchStrategy::MCMC )
    {
        proposePhasePatternUpdateRandomly();
    }
    else
    {
        // Walksat
        proposePhasePatternUpdateWalksat();
    }

    ASSERT( _currentPhasePattern != _lastAcceptedPhasePattern );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::NUM_PROPOSED_PHASE_PATTERN_UPDATE );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdateRandomly()
{
    SOI_LOG( "Proposing phase pattern update randomly..." );
    DEBUG( {
        // _plConstraintsInCurrentPhasePattern should contain the same
        // plConstraints in _currentPhasePattern
        ASSERT( _plConstraintsInCurrentPhasePattern.size() == _currentPhasePattern.size() );
        for ( const auto &pair : _currentPhasePattern )
            ASSERT( _plConstraintsInCurrentPhasePattern.exists( pair.first ) );
    } );

    // First, pick a pl constraint whose cost component we will update.
    bool fixed = true;
    PiecewiseLinearConstraint *plConstraintToUpdate;
    while ( fixed )
    {
        if ( _plConstraintsInCurrentPhasePattern.empty() )
            return;

        unsigned index = (unsigned)T::rand() % _plConstraintsInCurrentPhasePattern.size();
        plConstraintToUpdate = _plConstraintsInCurrentPhasePattern[index];
        fixed = plConstraintToUpdate->phaseFixed();
        if ( fixed )
            removeCostComponentFromHeuristicCost( plConstraintToUpdate );
    }

    // Next, pick an alternative phase.
    PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
    List<PhaseStatus> allPhases = plConstraintToUpdate->getAllCases();
    allPhases.erase( currentPhase );
    if ( allPhases.size() == 1 )
    {
        // There are only two possible phases. So we just flip the phase.
        _currentPhasePattern[plConstraintToUpdate] = *( allPhases.begin() );
    }
    else
    {
        auto it = allPhases.begin();
        unsigned index = (unsigned)T::rand() % allPhases.size();
        while ( index > 0 )
        {
            ++it;
            --index;
        }
        _currentPhasePattern[plConstraintToUpdate] = *it;
    }

    _constraintsUpdatedInLastProposal.append( plConstraintToUpdate );
    SOI_LOG( "Proposing phase pattern update randomly - done" );
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdateWalksat()
{
    SOI_LOG( "Proposing phase pattern update with Walksat-based strategy..." );
    obtainCurrentAssignment();

    // Flip to the cost term that reduces the cost by the most
    PiecewiseLinearConstraint *plConstraintToUpdate = NULL;
    PhaseStatus updatedPhase = PHASE_NOT_FIXED;
    double maxReducedCost = 0;
    for ( const auto &plConstraint : _plConstraintsInCurrentPhasePattern )
    {
        double reducedCost = 0;
        PhaseStatus phaseStatusOfReducedCost = PHASE_NOT_FIXED;
        getCostReduction( plConstraint, reducedCost, phaseStatusOfReducedCost );

        if ( reducedCost > maxReducedCost )
        {
            maxReducedCost = reducedCost;
            plConstraintToUpdate = plConstraint;
            updatedPhase = phaseStatusOfReducedCost;
        }
    }

    if ( plConstraintToUpdate )
    {
        _currentPhasePattern[plConstraintToUpdate] = updatedPhase;
        _constraintsUpdatedInLastProposal.append( plConstraintToUpdate );
    }
    else
    {
        proposePhasePatternUpdateRandomly();
    }
    SOI_LOG( "Proposing phase pattern update with Walksat-based strategy - done" );
}

bool SumOfInfeasibilitiesManager::decideToAcceptCurrentProposal( double costOfCurrentPhasePattern,
                                                                 double costOfProposedPhasePattern )
{
    if ( costOfProposedPhasePattern < costOfCurrentPhasePattern )
        return true;
    else
    {
        // The smaller the difference between the proposed phase pattern and the
        // current phase pattern, the more likely to accept the proposal.
        double prob = exp( -_probabilityDensityParameter *
                           ( costOfProposedPhasePattern - costOfCurrentPhasePattern ) );
        return ( (double)T::rand() / RAND_MAX ) < prob;
    }
}

void SumOfInfeasibilitiesManager::acceptCurrentPhasePattern()
{
    struct timespec start = TimeUtils::sampleMicro();

    _lastAcceptedPhasePattern = _currentPhasePattern;
    _constraintsUpdatedInLastProposal.clear();

    if ( _statistics )
    {
        _statistics->incLongAttribute( Statistics::NUM_ACCEPTED_PHASE_PATTERN_UPDATE );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SumOfInfeasibilitiesManager::updateCurrentPhasePatternForSatisfiedPLConstraints()
{
    obtainCurrentAssignment();
    for ( const auto &pair : _currentPhasePattern )
    {
        if ( pair.first->satisfied() )
        {
            PhaseStatus satisfiedPhaseStatus =
                pair.first->getPhaseStatusInAssignment( _currentAssignment );
            _currentPhasePattern[pair.first] = satisfiedPhaseStatus;
        }
    }
}

void SumOfInfeasibilitiesManager::removeCostComponentFromHeuristicCost(
    PiecewiseLinearConstraint *constraint )
{
    if ( _currentPhasePattern.exists( constraint ) )
    {
        _currentPhasePattern.erase( constraint );
        _lastAcceptedPhasePattern.erase( constraint );
        ASSERT( _plConstraintsInCurrentPhasePattern.exists( constraint ) );
        _plConstraintsInCurrentPhasePattern.erase( constraint );
    }
}

void SumOfInfeasibilitiesManager::obtainCurrentAssignment()
{
    struct timespec start = TimeUtils::sampleMicro();

    _currentAssignment.clear();
    for ( unsigned i = 0; i < _numberOfVariables; ++i )
        _currentAssignment[i] = _tableau.getValue( i );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SumOfInfeasibilitiesManager::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void SumOfInfeasibilitiesManager::setPhaseStatusInLastAcceptedPhasePattern(
    PiecewiseLinearConstraint *constraint,
    PhaseStatus phase )
{
    ASSERT( _lastAcceptedPhasePattern.exists( constraint ) &&
            _plConstraintsInCurrentPhasePattern.exists( constraint ) );
    _lastAcceptedPhasePattern[constraint] = phase;
}

void SumOfInfeasibilitiesManager::setPhaseStatusInCurrentPhasePattern(
    PiecewiseLinearConstraint *constraint,
    PhaseStatus phase )
{
    ASSERT( _currentPhasePattern.exists( constraint ) &&
            _plConstraintsInCurrentPhasePattern.exists( constraint ) );
    _currentPhasePattern[constraint] = phase;
}

void SumOfInfeasibilitiesManager::setPLConstraintsInCurrentPhasePattern(
    const Vector<PiecewiseLinearConstraint *> &constraints )
{
    _plConstraintsInCurrentPhasePattern = constraints;
}

void SumOfInfeasibilitiesManager::getCostReduction( PiecewiseLinearConstraint *plConstraint,
                                                    double &reducedCost,
                                                    PhaseStatus &phaseOfReducedCost ) const
{
    ASSERT( _currentPhasePattern.exists( plConstraint ) );
    SOI_LOG( "Computing reduced cost for the current constraint..." );

    // Get the list of alternative phases.
    PhaseStatus currentPhase = _currentPhasePattern[plConstraint];
    List<PhaseStatus> allPhases = plConstraint->getAllCases();
    allPhases.erase( currentPhase );
    ASSERT( allPhases.size() > 0 ); // Otherwise, the constraint must be fixed.
    SOI_LOG( Stringf( "Number of alternative phases: %u", allPhases.size() ).ascii() );

    // Compute the violation of the plConstraint w.r.t. the current assignment
    // and the current cost component.
    LinearExpression costComponent;
    plConstraint->getCostFunctionComponent( costComponent, currentPhase );
    double currentCost = costComponent.evaluate( _currentAssignment );

    // Next we iterate over alternative phases to see whether some phases
    // might reduce the cost and by how much.
    reducedCost = FloatUtils::infinity();
    phaseOfReducedCost = PHASE_NOT_FIXED;
    for ( const auto &phase : allPhases )
    {
        LinearExpression otherCostComponent;
        plConstraint->getCostFunctionComponent( otherCostComponent, phase );
        double otherCost = otherCostComponent.evaluate( _currentAssignment );
        double currentReducedCost = currentCost - otherCost;
        SOI_LOG( Stringf( "Reduced cost of phase %u: %.2f", currentReducedCost, phase ).ascii() );
        if ( FloatUtils::lt( currentReducedCost, reducedCost ) )
        {
            reducedCost = currentReducedCost;
            phaseOfReducedCost = phase;
        }
    }
    ASSERT( reducedCost != FloatUtils::infinity() && phaseOfReducedCost != PHASE_NOT_FIXED );
    SOI_LOG( Stringf( "Largest reduced cost is %.2f and achieved with phase status %u",
                      reducedCost,
                      phaseOfReducedCost )
                 .ascii() );
    SOI_LOG( "Computing reduced cost for the current constraint - done" );
}
