/*********************                                                        */
/*! \file SumOfInfeasibilitiesManager.h
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

#ifndef __SumOfInfeasibilitiesManager_h__
#define __SumOfInfeasibilitiesManager_h__

#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "ITableau.h"
#include "LinearExpression.h"
#include "List.h"
#include "NetworkLevelReasoner.h"
#include "PiecewiseLinearConstraint.h"
#include "SoIInitializationStrategy.h"
#include "SoISearchStrategy.h"
#include "Statistics.h"
#include "Vector.h"

#include <memory>
#include <random>

#define SOI_LOG( x, ... ) LOG( GlobalConfiguration::SOI_LOGGING, "SoIManager: %s\n", x )

class SumOfInfeasibilitiesManager
{
public:

    SumOfInfeasibilitiesManager( const InputQuery &inputQuery );

    /*
      Obtain the actual phase pattern from _currentPhasepattern
    */
    LinearExpression getSoIPhasePattern() const;

    /*
      Called at the beginning of the local search (DeepSoI).
      Choose the first phase pattern by heuristically taking a cost term
      from each unfixed activation function.
    */
    void initializePhasePattern();

    /*
      Called when the previous heuristic cost cannot be minimized to 0 (i.e., no
      satisfying assignment found for the previous activation pattern).
      In this case, we need to try a new phase pattern. We achieve this by
      propose an update to the previous phase pattern,
      stored in _currentProposal.
    */
    void proposePhasePatternUpdate();

    /*
      The acceptance heuristic is standard: if the newCost is less than
      _costOfCurrentphasepattern, we always accept. Otherwise, the probability
      to accept the proposal is reversely proportional to the difference between
      the newCost and the _costOfcurrentphasepattern.
    */
    bool decideToAcceptCurrentProposal( double costOfCurrentPhasePattern,
                                        double costOfProposedPhasePattern );

    /*
      Set the _currentPhasePattern to be _currentPhasePattern + _currentProposal.
      Then clear the _currentProposal.
    */
    void acceptCurrentProposal();

    // Go through each PLConstraint, check whether it is satisfied by the
    // current assignment but the cost term is not zero. In that case,
    // we use the cost term corresponding to the phase of the current assignment
    // for that PLConstraint. This way, the cost term is trivially minimized.
    void updateCurrentPhasePatternForSatisfiedPLConstraints();

    // During the Simplex execution, the phase of a piecewise linear constraint
    // might be fixed due to additional tightening.
    // In that case, we remove the cost term for that piecewise linear constraint
    // from the heuristic cost.
    inline void removeCostComponentFromHeuristicCost( PiecewiseLinearConstraint
                                                      *constraint )
    {
        if ( _currentPhasePattern.exists( constraint ) )
            _currentPhasePattern.erase( constraint );
    }

    // Compute _currentPhasePattern from the current variable assignment.
    double computeHeuristicCost();

    void setStatistics( Statistics *statistics );

    /* Debug only */
    void dumpHeuristicCost();

private:
    friend class Test_SumOfInfeasibilitiesManager;

    const List<PiecewiseLinearConstraint *> &_plConstraints;
    NLR::NetworkLevelReasoner *_networkLevelReasoner;

    SoIInitializationStrategy _initializationStrategy;
    SoISearchStrategy _searchStrategy;

    Map<PiecewiseLinearConstraint *, PhaseStatus> _currentPhasePattern;
    Map<PiecewiseLinearConstraint *, PhaseStatus> _currentProposal;

    Statistics *_statistics;

    /*
      Clear _currentPhasePattern and _currentProposal
    */
    void resetPhasePattern();

    /*
      Set _currentPhasePattern according to the current input assignment.
    */
    void initializePhasePatternWithCurrentInputAssignment();

};

#endif // __SumOfInfeasibilitiesManager_h__
