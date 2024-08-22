/*********************                                                        */
/*! \file SumOfInfeasibilitiesManager.h
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

#ifndef __SumOfInfeasibilitiesManager_h__
#define __SumOfInfeasibilitiesManager_h__

#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "LinearExpression.h"
#include "List.h"
#include "NetworkLevelReasoner.h"
#include "PiecewiseLinearConstraint.h"
#include "Query.h"
#include "SoIInitializationStrategy.h"
#include "SoISearchStrategy.h"
#include "Statistics.h"
#include "T/stdlib.h"
#include "Vector.h"

#define SOI_LOG( x, ... ) LOG( GlobalConfiguration::SOI_LOGGING, "SoIManager: %s\n", x )

class SumOfInfeasibilitiesManager
{
public:
    SumOfInfeasibilitiesManager( const Query &inputQuery, const ITableau &tableau );

    /*
      Returns the actual current phase pattern from _currentPhasePattern
    */
    LinearExpression getCurrentSoIPhasePattern() const;

    /*
      Returns the actual current phase pattern from _lastAcceptedPhasePattern
    */
    LinearExpression getLastAcceptedSoIPhasePattern() const;

    /*
      Return the list of constraints updated in the last proposal.
      This list is updated during proposePhasePatternUpdate().
    */
    inline const List<PiecewiseLinearConstraint *> &getConstraintsUpdatedInLastProposal() const
    {
        return _constraintsUpdatedInLastProposal;
    }

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
      proposing an update to the previous phase pattern,
      stored in _currentPhasePattern.
    */
    void proposePhasePatternUpdate();

    /*
      The acceptance heuristic is standard: if the newCost is less than
      the current cost, we always accept. Otherwise, the probability
      to accept the proposal is reversely proportional to the difference between
      the newCost and the _costOfCurrentPhasePattern.
    */
    bool decideToAcceptCurrentProposal( double costOfCurrentPhasePattern,
                                        double costOfProposedPhasePattern );

    /*
      Set _lastAcceptedPhasePattern to be _currentPhasePattern
    */
    void acceptCurrentPhasePattern();

    // Go through each PLConstraint, check whether it is satisfied by the
    // current assignment but the cost term is not zero. In that case,
    // we use the cost term corresponding to the phase of the current assignment
    // for that PLConstraint. This way, the overall SoI cost is reduced for free.
    void updateCurrentPhasePatternForSatisfiedPLConstraints();

    // During the Simplex execution, the phase of a piecewise linear constraint
    // might be fixed due to additional tightening.
    // In that case, we remove the cost term for that piecewise linear constraint
    // from the heuristic cost.
    void removeCostComponentFromHeuristicCost( PiecewiseLinearConstraint *constraint );

    /*
      Obtain the current variable assignment from the Tableau.
    */
    void obtainCurrentAssignment();

    void setStatistics( Statistics *statistics );

    /* For debug use */
    void setPhaseStatusInLastAcceptedPhasePattern( PiecewiseLinearConstraint *constraint,
                                                   PhaseStatus phase );

    void setPhaseStatusInCurrentPhasePattern( PiecewiseLinearConstraint *constraint,
                                              PhaseStatus phase );

    void
    setPLConstraintsInCurrentPhasePattern( const Vector<PiecewiseLinearConstraint *> &constraints );

private:
    const List<PiecewiseLinearConstraint *> &_plConstraints;
    // Used for the heuristic initialization of the phase pattern.
    NLR::NetworkLevelReasoner *_networkLevelReasoner;
    unsigned _numberOfVariables;
    // Used for accessing the current variable assignment.
    const ITableau &_tableau;

    // Parameters that controls the local search heuristics
    SoIInitializationStrategy _initializationStrategy;
    SoISearchStrategy _searchStrategy;
    double _probabilityDensityParameter;

    /*
      The representation of the current phase pattern (one linear phase of the
      non-linear SoI function) as a mapping from PLConstraints to phase patterns.
      We do not keep the concrete LinearExpression explicitly but will concretize
      it on the fly. This makes it cheap to update the phase pattern.
    */
    Map<PiecewiseLinearConstraint *, PhaseStatus> _currentPhasePattern;

    /*
      The most recently accepted phase pattern.
    */
    Map<PiecewiseLinearConstraint *, PhaseStatus> _lastAcceptedPhasePattern;

    /*
      The constraints in the current phase pattern (i.e., participating in the
      SoI) stored in a Vector for ease of random access.
    */
    Vector<PiecewiseLinearConstraint *> _plConstraintsInCurrentPhasePattern;

    /*
      A local copy of the current variable assignment, which is refreshed via
      the obtainCurrentAssignment() method.
    */
    Map<unsigned, double> _currentAssignment;

    /*
      The constraints whose cost terms were changed in the last proposal.
      We keep track of this for the PseudoImpact branching heuristics.
    */
    List<PiecewiseLinearConstraint *> _constraintsUpdatedInLastProposal;

    Statistics *_statistics;

    /*
      Clear _currentPhasePattern, _lastAcceptedPhasePattern and
      _plConstraintsInCurrentPhasePattern.
    */
    void resetPhasePattern();

    /*
      Set _currentPhasePattern according to the current input assignment.
    */
    void initializePhasePatternWithCurrentInputAssignment();

    /*
      Set _currentPhasePattern according to the current assignment.
    */
    void initializePhasePatternWithCurrentAssignment();

    /*
      Choose one piecewise linear constraint in the current phase pattern
      and set it to a uniform-randomly chosen alternative phase status (for ReLU
      this means we just flip the phase status).
    */
    void proposePhasePatternUpdateRandomly();

    /*
      Iterate over the piecewise linear constraints in the current phase pattern
      to find one with the largest "cost reduction". See the "getCostReduction"
      method below.
      If no constraint has positive cost reduction (we are at a local optima), we
      fall back to proposePhasePatternUpdateRandomly()
    */
    void proposePhasePatternUpdateWalksat();

    /*
      This method computes the cost reuduction of a plConstraint participating
      in the phase pattern. The cost reduction is the largest value by which the
      cost (w.r.t. the current assignment) will decrease if we choose a
      different phase for the plConstraint in the phase pattern. This value is
      stored in reducedCost. The phase corresponding to the largest reduction
      is stored in phaseOfReducedCost.
      Note that the phase can be negative, which means the current phase is
      (locally) optimal.
    */
    void getCostReduction( PiecewiseLinearConstraint *plConstraint,
                           double &reducedCost,
                           PhaseStatus &phaseOfReducedCost ) const;
};

#endif // __SumOfInfeasibilitiesManager_h__
