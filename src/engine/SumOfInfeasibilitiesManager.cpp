/*********************                                                        */
/*! \file SumOfInfeasibilitiesManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "MarabouError.h"
#include "Options.h"
#include "SumOfInfeasibilitiesManager.h"

#include "Set.h"

SumOfInfeasibilitiesManager::SumOfInfeasibilitiesManager( const InputQuery
                                                          &inputQuery,
                                                          const ITableau
                                                          &tableau )
    : _plConstraints( inputQuery.getPiecewiseLinearConstraints() )
    , _networkLevelReasoner( inputQuery.getNetworkLevelReasoner() )
    , _tableau( tableau )
    , _initializationStrategy( Options::get()->getSoIInitializationStrategy() )
    , _searchStrategy( Options::get()->getSoISearchStrategy() )
    , _probabilityDensityParameter( Options::get()->getFloat
                                    ( Options::PROBABILITY_DENSITY_PARAMETER ) )
{}

void SumOfInfeasibilitiesManager::resetPhasePattern()
{
    _currentPhasePattern.clear();
    _currentProposal.clear();
    _plConstraintsInCurrentPhasePattern.clear();
}

LinearExpression SumOfInfeasibilitiesManager::getSoIPhasePattern() const
{
    LinearExpression cost;
    for ( const auto &pair : _currentPhasePattern )
    {
        pair.first->getCostFunctionComponent( cost, pair.second );
    }
    return cost;
}

LinearExpression SumOfInfeasibilitiesManager::getProposedSoIPhasePattern() const
{
    DEBUG({
            // Check that the constraints in the proposal is a subset of those
            // in the currentPhasePattern
            ASSERT( Set<PiecewiseLinearConstraint *>::containedIn
                    ( _currentProposal.keys(), _currentPhasePattern.keys() ) );
        });

    LinearExpression cost;
    for ( const auto &pair : _currentProposal )
        pair.first->getCostFunctionComponent( cost, pair.second );

    for ( const auto &pair : _currentPhasePattern )
        if ( !_currentProposal.exists( pair.first ) )
            pair.first->getCostFunctionComponent( cost, pair.second );

    return cost;
}

void SumOfInfeasibilitiesManager::initializePhasePattern()
{
    resetPhasePattern();
    if ( _initializationStrategy == SoIInitializationStrategy::INPUT_ASSIGNMENT
         && _networkLevelReasoner )
    {
        initializePhasePatternWithCurrentInputAssignment();
    }
    else
    {
        throw MarabouError
            ( MarabouError::UNABLE_TO_INITIALIZATION_PHASE_PATTERN );
    }
    for ( const auto &pair : _currentPhasePattern )
        _plConstraintsInCurrentPhasePattern.append( pair.first );
}

void SumOfInfeasibilitiesManager::initializePhasePatternWithCurrentInputAssignment()
{
    ASSERT( _networkLevelReasoner );
    Map<unsigned, double> assignment;
    _networkLevelReasoner->concretizeInputAssignment( assignment );
    std::cout << "Assignment: " << std::endl;
    for ( const auto pair : assignment )
        std::cout << pair.first << " " << pair.second << std::endl;

    for ( const auto &plConstraint : _plConstraints )
    {
        ASSERT( !_currentPhasePattern.exists( plConstraint ) );
        if ( plConstraint->isActive() && !plConstraint->phaseFixed() )
        {
            _currentPhasePattern[plConstraint] =
                plConstraint->getPhaseStatusInAssignment( assignment );
        }
    }
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdate()
{
    _currentProposal.clear();
    if ( _searchStrategy == SoISearchStrategy::MCMC )
    {
        proposePhasePatternUpdateRandomly();
    }
    else
    {
        // Walksat
        proposePhasePatternUpdateWalksat();
    }
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdateRandomly()
{
    DEBUG({
            // _plConstraintsInCurrentPhasePattern should contain the same
            // plConstraints in _currentPhasePattern
            ASSERT( _plConstraintsInCurrentPhasePattern.size() ==
                    _currentPhasePattern.size() );
            for ( const auto &pair : _currentPhasePattern )
                ASSERT( _plConstraintsInCurrentPhasePattern.exists
                        ( pair.first ) );
        });

    unsigned index = ( unsigned ) rand() %
                       _plConstraintsInCurrentPhasePattern.size();
    PiecewiseLinearConstraint *plConstraintToUpdate =
        _plConstraintsInCurrentPhasePattern[index];
    PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
    List<PhaseStatus> allPhases = plConstraintToUpdate->getAllCases();
    allPhases.erase( currentPhase );
    if ( allPhases.size() == 1 )
    {
        // There are only two possible phases. So we just flip the phase.
        _currentProposal[plConstraintToUpdate] = *( allPhases.begin() );
    }
    else
    {
        auto it = allPhases.begin();
        unsigned index =  ( unsigned ) rand() % allPhases.size();
        while ( index > 0 )
        {
            ++it;
            --index;
        }
        _currentProposal[plConstraintToUpdate] = *it;
    }
}

void SumOfInfeasibilitiesManager::proposePhasePatternUpdateWalksat()
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

bool SumOfInfeasibilitiesManager::decideToAcceptCurrentProposal
( double costOfCurrentPhasePattern, double costOfProposedPhasePattern )
{
    if ( costOfProposedPhasePattern < costOfCurrentPhasePattern )
        return true;
    else
    {
        // The smaller the difference between the proposed phase pattern and the
        // current phase pattern, the more likely to accept the proposal.
        double prob = exp( -_probabilityDensityParameter *
                           ( costOfProposedPhasePattern -
                             costOfCurrentPhasePattern ) );
        return ( (float) rand() / RAND_MAX ) < prob;
    }
}


void SumOfInfeasibilitiesManager::acceptCurrentProposal()
{
    // We update _currentPhasePattern with entries in _currentProposal
    for ( const auto &pair : _currentProposal )
    {
        _currentPhasePattern[pair.first] = pair.second;
    }
}

void SumOfInfeasibilitiesManager::updateCurrentPhasePatternForSatisfiedPLConstraints()
{
    //for ( const auto &pair : _currentPhasePattern )
    //   {
    //      if ( pair.first->satisfied() )
}

void SumOfInfeasibilitiesManager::removeCostComponentFromHeuristicCost
( PiecewiseLinearConstraint *constraint )
{
    if ( _currentPhasePattern.exists( constraint ) )
    {
        _currentPhasePattern.erase( constraint );
        ASSERT( _plConstraintsInCurrentPhasePattern.exists( constraint ) );
        _plConstraintsInCurrentPhasePattern.erase( constraint );
    }
}

void SumOfInfeasibilitiesManager::getReducedCost( double &reducedCost,
                                                  PhaseStatus
                                                  &phaseOfReducedCost ) const
{
    //List<PhaseStatus> allPhases = plConstraintToUpdate->getAllCases();
    //allPhases.erase( currentPhase );
    reducedCost = 0;
    phaseOfReducedCost = PHASE_NOT_FIXED;
}
