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
#include "IEngine.h"
#include "LinearExpression.h"
#include "List.h"
#include "PiecewiseLinearConstraint.h"
#include "SoIInitializationStrategy.h"
#include "SoISearchStrategy.h"
#include "Statistics.h"
#include "Vector.h"

#include <memory>
#include <random>

#define SOI_LOG( x, ... ) LOG( GlobalConfiguration::SUM_OF_INFEASIBILITIES_LOGGING, "SoIManager: %s\n", x )

class SumOfInfeasibilitiesManager
{
public:

    SumOfInfeasibilitiesManager( IEngine *engine );

    inline const Map<unsigned, double> &getHeuristicCost() const
    {
        return _heuristicCost;
    }

    /*
      Called at the beginning of the local search (DeepSoI).
      Choose the first phase pattern by heuristically taking a cost term
      from each unfixed activation function.
    */
    void initializeHeuristicCost();

    /*
      Called when the previous heuristic cost cannot be minimized to 0 (i.e., no
      satisfying assignment found for the previous activation pattern).
      In this case, we need to try a new phase pattern. We achieve this by
      propose an update to the previous phase pattern,
      stored in _currentProposal.
    */
    void proposeHeuristicCostUpdate();

    /*
      Given the minimal value of the proposed new phase pattern
      (_currentPhasepattern + _currentProposal). Returns true if we are going
      to accept this proposal.
      The acceptance heuristic is standard: if the newCost is less than
      _costOfCurrentphasepattern, we always accept. Otherwise, the probability
      to accept the proposal is reversely proportional to the difference between
      the newCost and the _costOfcurrentphasepattern.
    */
    bool decideOnCurrentProposal( double newCost );

    /*
      Set the _currentPhasePattern to be _currentPhasePattern + _currentProposal.
    */
    void acceptCurrentProposal();

    // Go through each PLConstraint, check whether it is satisfied by the
    // current assignment but the cost term is not zero. In that case,
    // we use the cost term corresponding to the phase of the current assignment
    // for that PLConstraint. This way, the cost term is trivially minimized.
    void updateCostTermsForSatisfiedPLConstraints();

    // During the Simplex execution, the phase of a piecewise linear constraint
    // might be fixed due to additional tightening.
    // In that case, we remove the cost term for that piecewise linear constraint
    // from the heuristic cost.
    void removeCostComponentFromHeuristicCost( PiecewiseLinearConstraint *constraint );

    // Compute the heuristic cost from the current assignment.
    double computeHeuristicCost();

    void setStatistics( Statistics *statistics );

    void setPLConstraints( List<PiecewiseLinearConstraint *> &plConstraints );

    void setNetworkLevelReasoner( NLR::NetworkLevelReasoner *networkLevelReasoner );

    /* Debug only */
    void dumpHeuristicCost();

private:

    // The current heuristic cost.
    // If it can be minimized to 0 w.r.t. the convex relaxation (the current set
    // of linear constraints), then a satisfying assignment is found.
    LinearExpression _currentPhasePattern;

    // The lastest proposed update to obtain the _currentPhasePattern
    // _newPhasePattern = _currentPhasePattern + _currentProposal
    LinearExpression _currentProposal;

    // The minimal value of _currentPhasePattern. Updated after the convex solver
    // call on the _currentPhasePattern.
    double _costOfCurrentPhasePattern;

    Statistics *_statistics;

};

#endif // __SumOfInfeasibilitiesManager_h__
