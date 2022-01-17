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
      Initialize the SoI function by heuristically taking a cost term
      from each unfixed activation function.
    */
    void initializeHeuristicCost();

    /*
      Called when the previous heuristic cost cannot be minimized to 0 (i.e., no
      satisfying assignment found for the previous activation pattern).
      In this case, we need to try a new heuristic cost. We achieve this by
      propose an update to the previous heuristic cost (e.g., by using a different
      cost term for certain ReLU).
    */
    void proposeHeuristicCostUpdate();

    /*
      We have computed the minimal value of the previous heuristic cost (previousCost),
      as well as the minimal value of the currently proposed heuristic cost (currentCost).
      This method decides whether we accept this proposal or reject this proposal.
      In the latter case, we need to undo the last proposal and revert the _heuristicCost
      back to the previous one (using the undoLastProposal method).
    */
    bool acceptLastProposal( double previousCost, double currentCost );

    /*
      Undo the last proposal. By subtracting _lastProposal from _heuristicCost
    */
    void undoLastProposal();

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

    Map<unsigned, double> _heuristicCost;
    Map<unsigned, double> _lastProposal;
};

#endif // __SumOfInfeasibilitiesManager_h__
