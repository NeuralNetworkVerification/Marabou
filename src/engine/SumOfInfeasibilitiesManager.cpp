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

SumOfInfeasibilitiesManager::SumOfInfeasibilitiesManager( const InputQuery
                                                          &inputQuery, const
                                                          ITableau &tableau )
    : _plConstraints( inputQuery.getPiecewiseLinearConstraints() )
    , _tableau( tableau )
    , _networkLevelReasoner( inputQuery.getNetworkLevelReasoner() )
    , _initializationStrategy( Options::get()->getSoIInitializationStrategy() )
    , _searchStrategy( Options::get()->getSoISearchStrategy() )
{}

LinearExpression SumOfInfeasibilitiesManager::getSoIPhasePattern() const
{
    LinearExpression cost;
    for ( const auto &pair : _currentPhasePattern )
    {
        pair.first->getCostFunctionComponent( cost, pair.second );
    }
    return cost;
}

void SumOfInfeasibilitiesManager::initializePhasePattern()
{
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
}

void SumOfInfeasibilitiesManager::initializePhasePatternWithCurrentInputAssignment()
{
    ASSERT( _networkLevelReasoner );
}
