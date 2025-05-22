/*********************                                                        */
/*! \file CdclCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2025 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __CdclCore_h__
#define __CdclCore_h__

#ifdef BUILD_CADICAL

#include "CadicalWrapper.h"
#include "IEngine.h"
#include "InputQuery.h"
#include "PLConstraintScoreTracker.h"
#include "Pair.h"
#include "PiecewiseLinearConstraint.h"
#include "Statistics.h"
#include "context/cdhashmap.h"
#include "context/cdhashset.h"

#include <cadical.hpp>

#define CDCL_LOG( x, ... ) LOG( GlobalConfiguration::CDCL_LOGGING, "CDCL: %s\n", x )

using CVC4::context::Context;

typedef Set<int> Clause;

class CdclCore
    : CaDiCaL::ExternalPropagator
    , CaDiCaL::Terminator
    , CaDiCaL::FixedAssignmentListener
{
public:
    explicit CdclCore( IEngine *engine );
    ~CdclCore() override;

    /*
      Have the CDCL core start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

    /*
      Initializes the boolean abstraction for a PiecewiseLinearConstraint object
    */
    void initBooleanAbstraction( PiecewiseLinearConstraint *plc );

    /*
       Push _context, record statistics
     */
    void pushContext();

    /*
       Pop _context to given level, record statistics
     */
    void popContextTo( unsigned level );

    /*
      Add valid literal to clause or zero to terminate clause.
    */
    void addLiteral( int lit );

    /*
        Calls the solving method combining the SAT solver with Marabou back engine
    */
    bool solveWithCDCL( double timeoutInSeconds );

    /**********************************************************************/
    /*  IPASIR-UP functions, for integrating Marabou with the SAT solver  */
    /**********************************************************************/

    /*
      Notify Marabou about an assignment of a boolean (abstract) variable
    */
    void notify_assignment( const std::vector<int> &lits ) override;

    /*
      Notify Marabou about a new decision level
    */
    void notify_new_decision_level() override;

    /*
      Notify Marabou should backtrack to new_level decision level
    */
    void notify_backtrack( size_t new_level ) override;

    /*
      Callback from the SAT solver that calls Marabou to check a full assignment of the boolean
      (abstract) variables
    */
    bool cb_check_found_model( const std::vector<int> &model ) override;

    /*
      Callback from the SAT solver that allows Marabou to decide a boolean (abstract) variable to
      split on
    */
    int cb_decide() override;

    /*
      Callback from the SAT solver that enables Marabou propagate literals leanred based on a
      partial assignment
     */
    int cb_propagate() override;

    /*
      Callback from the SAT solver that requires Marabou to explain a propagation.
      Returns a literal in the explanation clause one at a time, including the literal to explain.
      Ends with 0.
    */
    int cb_add_reason_clause_lit( int propagated_lit ) override;

    /*
      Check if Marabou has a conflict clause to inform the SAT solver
    */
    bool cb_has_external_clause( bool &is_forgettable ) override;
    /*
      Add conflict clause from Marabou to the SAT solver, one literal at a time. Ends with 0.
    */
    int cb_add_external_clause_lit() override;

    /*
      Internally adds a conflict clause when learned, later to be informed to the SAT solver
    */
    void addExternalClause( Set<int> &clause );

    /*
       Returns the PiecewiseLinearConstraint abstraced by the literal lit
    */
    const PiecewiseLinearConstraint *getConstraintFromLit( int lit ) const;

    /*
      Internally adds a literal, when learned, later to be informed to the SAT solver
    */
    void addLiteralToPropagate( int literal );

    /*
      Adds the decision-based conflict clause (negation of all decisions), except the given literal,
      to Marabou, later to be propagated
    */
    void addDecisionBasedConflictClause();

    /*
     Remove a literal from the propagation list
    */
    void removeLiteralFromPropagations( int literal );

    /*
      Force the default decision phase of a variable to a certain value.
     */
    void phase( int literal );

    /*
      Check if the solver should stop due to the requested timeout by the user
     */
    void checkIfShouldExitDueToTimeout();

    /*
      Connected terminators are checked for termination regularly.  If the
      'terminate' function of the terminator returns true the solver is
      terminated synchronously as soon it calls this function.
     */
    bool terminate() override;

    /*
     Get the index of an assigned literal in the assigned literals list
     return the size of the list if element not found
     */
    unsigned getLiteralAssignmentIndex( int literal );

    /*
      Return true iff the literal is fixed by the SAT solver
    */
    bool isLiteralFixed( int literal ) const;

    /*
  Notifying on a fixed literal assignment.
 */
    void notify_fixed_assignment( int lit ) override;

    /*
      Notify about a single assignment
     */
    void notifySingleAssignment( int lit, bool isFixed );

    /*
      Returns true if a conflict clause exists
     */
    bool hasConflictClause() const;

    /*
      Check if the given piecewise-linear constraint is currently supported by CDCL
     */
    static bool isSupported( const PiecewiseLinearConstraint *plc );

    /*
      Initialize score stracker for pseudo-impact based decisions.
     */
    void initializeScoreTracker( std::shared_ptr<PLConstraintScoreTracker> scoreTracker );

    bool isDecision( int lit );

private:
    /*
      The engine.
    */
    IEngine *_engine;

    /*
      Context for synchronizing the search.
     */
    Context &_context;

    /*
      Collect and print various statistics.
    */
    Statistics *_statistics;

    /*
      SAT solver object
    */
    SatSolverWrapper *_satSolverWrapper;

    /*
      Boolean abstraction map, from boolean variables to the PiecewiseLinearConstraint they
      represent
    */
    Map<unsigned, PiecewiseLinearConstraint *> _cadicalVarToPlc;

    /*
      Internal data structures to keep track of literals to propagate, assigned and fixed literals;
      and reason and conflict clauses
    */
    List<Pair<int, int>> _literalsToPropagate;
    CVC4::context::CDHashMap<int, unsigned> _assignedLiterals;

    Vector<int> _reasonClauseLiterals;
    bool _isReasonClauseInitialized;

    Vector<int> _externalClauseToAdd;

    Set<int> _fixedCadicalVars;

    double _timeoutInSeconds;

    unsigned _numOfClauses;
    CVC4::context::CDHashSet<unsigned, std::hash<unsigned>> _satisfiedClauses;
    Map<int, Set<unsigned>> _literalToClauses;
    unsigned _vsidsDecayThreshold;
    unsigned _vsidsDecayCounter;

    unsigned _restarts;
    unsigned _restartLimit;
    unsigned _numOfConflictClauses;
    bool _shouldRestart;

    HashMap<unsigned, bool> _largestAssignmentSoFar;

    Vector<Set<int>> _initialClauses;

    std::shared_ptr<PLConstraintScoreTracker> _scoreTracker;

    Map<unsigned, int> _decisionLiterals;
    Map<unsigned, double> _storedLowerBounds;
    Map<unsigned, double> _storedUpperBounds;

    Map<int, double> _decisionScores;

    /*
     Decision heuristics
     */
    unsigned decideSplitVarBasedOnPolarityAndVsids() const;
    unsigned decideSplitVarBasedOnPseudoImpactAndVsids() const;


    /*
      Access info in the internal data structures
    */
    bool isLiteralAssigned( int literal ) const;
    bool isLiteralToBePropagated( int literal ) const;

    bool isClauseSatisfied( unsigned clause ) const;
    unsigned int getLiteralVSIDSScore( int literal ) const;
    unsigned int getVariableVSIDSScore( unsigned var ) const;

    unsigned luby( unsigned i );

    double computeDecisionScoreForLiteral( int literal ) const;
    void setInputBoundsForLiteralInNLR( int literal,
                                        const std::shared_ptr<Query>& inputQuery,
                                        NLR::NetworkLevelReasoner *networkLevelReasoner ) const;
    void runSymbolicBoundTightening( NLR::NetworkLevelReasoner *networkLevelReasoner ) const;

    double
    getUpperBoundForOutputVariableFromNLR( NLR::NetworkLevelReasoner *networkLevelReasoner ) const;

    void computeClauseScores( const Set<int> &clause, Vector<Pair<double, int>> &clauseScores );
    void reorderByDecisionLevelIfNecessary( Vector<Pair<double, int>> &clauseScores );
    void computeShortedClause( Set<int> &clause,
                               const Vector<Pair<double, int>> &clauseScores ) const;
    bool checkIfShouldSkipClauseShortening( const Set<int> &clause );
};

#endif
#endif // __CdclCore_h__
