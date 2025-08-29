/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Engine_h__
#define __Engine_h__

#include "AutoCostFunctionManager.h"
#include "AutoProjectedSteepestEdge.h"
#include "AutoRowBoundTightener.h"
#include "AutoTableau.h"
#include "BlandsRule.h"
#include "BoundManager.h"
#include "Checker.h"
#include "DantzigsRule.h"
#include "DegradationChecker.h"
#include "DivideStrategy.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "IEngine.h"
#include "IQuery.h"
#include "JsonWriter.h"
#include "LPSolverType.h"
#include "LinearExpression.h"
#include "MILPEncoder.h"
#include "Map.h"
#include "Options.h"
#include "PrecisionRestorer.h"
#include "Preprocessor.h"
#include "Query.h"
#include "SignalHandler.h"
#include "SmtCore.h"
#include "SmtLibWriter.h"
#include "SnCDivideStrategy.h"
#include "SparseUnsortedList.h"
#include "Statistics.h"
#include "SumOfInfeasibilitiesManager.h"
#include "SymbolicBoundTighteningType.h"
#include "UnsatCertificateNode.h"

#include <atomic>
#include <context/context.h>


#ifdef _WIN32
#undef ERROR
#endif

#define ENGINE_LOG( x, ... ) LOG( GlobalConfiguration::ENGINE_LOGGING, "Engine: %s\n", x )

class EngineState;
class Query;
class PiecewiseLinearConstraint;
class String;


using CVC4::context::Context;

class Engine
    : public IEngine
    , public SignalHandler::Signalable
{
public:
    enum {
        MICROSECONDS_TO_SECONDS = 1000000,
    };

    Engine();
    ~Engine();

    /*
      Attempt to find a feasible solution for the input within a time limit
      (a timeout of 0 means no time limit). Returns true if found, false if infeasible.
    */
    bool solve( double timeoutInSeconds = 0 );

    /*
      Minimize the cost function with respect to the current set of linear constraints.
    */
    void minimizeHeuristicCost( const LinearExpression &heuristicCost );

    /*
      Compute the cost function with the current assignment.
    */
    double computeHeuristicCost( const LinearExpression &heuristicCost );

    /*
      Process the input query and pass the needed information to the
      underlying tableau. Return false if query is found to be infeasible,
      true otherwise.
     */
    bool processInputQuery( const IQuery &inputQuery );
    bool processInputQuery( const IQuery &inputQuery, bool preprocess );

    Query prepareSnCQuery();
    void exportQueryWithError( String errorMessage );

    /*
      Methods for calculating bounds.
    */
    bool calculateBounds( const IQuery &inputQuery );

    /*
      Method for extracting the bounds.
     */
    void extractBounds( IQuery &inputQuery );

    /*
      If the query is feasiable and has been successfully solved, this
      method can be used to extract the solution.
     */
    void extractSolution( IQuery &inputQuery, Preprocessor *preprocessor = nullptr );

    /*
      Methods for storing and restoring the state of the engine.
    */
    void storeState( EngineState &state, TableauStateStorageLevel level ) const;
    void restoreState( const EngineState &state );
    void setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints );

    /*
      Preprocessor access.
    */
    bool preprocessingEnabled() const;
    Preprocessor *getPreprocessor();

    /*
      A request from the user to terminate
    */
    void quitSignal();

    const Statistics *getStatistics() const;

    Query *getQuery();

    Query buildQueryFromCurrentState() const;

    /*
      Get the exit code
    */
    Engine::ExitCode getExitCode() const;

    /*
      Get the quitRequested flag
    */
    std::atomic_bool *getQuitRequested();

    /*
      Get the list of input variables
    */
    List<unsigned> getInputVariables() const;

    /*
      Add equations and tightenings from a split.
    */
    void applySplit( const PiecewiseLinearCaseSplit &split );

    /*
      Hooks invoked before/after context push/pop to store/restore/update context independent data.
    */
    void postContextPopHook();
    void preContextPushHook();

    /*
      Reset the state of the engine, before solving a new query
      (as part of DnC mode).
    */
    void reset();

    /*
      Reset the statistics object
    */
    void resetStatistics();

    /*
      Clear the violated PL constraints
    */
    void clearViolatedPLConstraints();

    /*
      Set the Engine's level of verbosity
    */
    void setVerbosity( unsigned verbosity );

    /*
      Apply the stack to the newly created SmtCore, returns false if UNSAT is
      found in this process.
    */
    bool restoreSmtState( SmtState &smtState );

    /*
      Store the current stack of the smtCore into smtState
    */
    void storeSmtState( SmtState &smtState );

    /*
      Pick the piecewise linear constraint for splitting
    */
    PiecewiseLinearConstraint *pickSplitPLConstraint( DivideStrategy strategy );

    /*
      Call-back from QueryDividers
      Pick the piecewise linear constraint for splitting
    */
    PiecewiseLinearConstraint *pickSplitPLConstraintSnC( SnCDivideStrategy strategy );

    /*
      PSA: The following two methods are for DnC only and should be used very
      cautiously.
     */
    void resetSmtCore();
    void resetExitCode();
    void resetBoundTighteners();

    /*
       Register initial split when in SnC mode
     */
    void applySnCSplit( PiecewiseLinearCaseSplit sncSplit, String queryId );

    bool inSnCMode() const;

    /*
       Apply bound tightenings stored in the bound manager.
     */
    void applyBoundTightenings();

    /*
      Apply all bound tightenings (row and matrix-based) in
      the queue.
    */
    void applyAllBoundTightenings();

    /*
      Apply all valid case splits proposed by the constraints.
      Return true if a valid case split has been applied.
    */
    bool applyAllValidConstraintCaseSplits();

    void setRandomSeed( unsigned seed );

    /*
      Returns true iff the engine is in proof production mode
    */
    bool shouldProduceProofs() const;

    /*
      Update the ground bounds
    */
    void updateGroundUpperBound( unsigned var, double value );
    void updateGroundLowerBound( unsigned var, double value );

    /*
      Return all ground bounds as a vector
    */
    double getGroundBound( unsigned var, bool isUpper ) const;

    /*
      Get the current pointer of the UNSAT certificate
    */
    UnsatCertificateNode *getUNSATCertificateCurrentPointer() const;

    /*
     Set the current pointer of the UNSAT certificate
    */
    void setUNSATCertificateCurrentPointer( UnsatCertificateNode *node );

    /*
      Get the pointer to the root of the UNSAT certificate
    */
    const UnsatCertificateNode *getUNSATCertificateRoot() const;

    /*
      Certify the UNSAT certificate
    */
    bool certifyUNSATCertificate();

    /*
      Get the boundExplainer
    */
    const BoundExplainer *getBoundExplainer() const;

    /*
      Set the boundExplainer
    */
    void setBoundExplainerContent( BoundExplainer *boundExplainer );

    /*
      Propagate bound tightenings stored in the BoundManager
    */
    void propagateBoundManagerTightenings();

    /*
      Add lemma to the UNSAT Certificate
    */
    void addPLCLemma( std::shared_ptr<PLCLemma> &explanation );

private:
    enum BasisRestorationRequired {
        RESTORATION_NOT_NEEDED = 0,
        STRONG_RESTORATION_NEEDED = 1,
        WEAK_RESTORATION_NEEDED = 2
    };

    enum BasisRestorationPerformed {
        NO_RESTORATION_PERFORMED = 0,
        PERFORMED_STRONG_RESTORATION = 1,
        PERFORMED_WEAK_RESTORATION = 2,
    };


    /*
      Perform bound tightening operations that require
      access to the explicit basis matrix.
    */
    void explicitBasisBoundTightening();

    /*
       Context is the central object that manages memory and back-tracking
       across context-dependent components - SMTCore,
       PiecewiseLinearConstraints, BoundManager, etc.
     */
    Context _context;

    /*
       BoundManager is the centralized context-dependent object that stores
       derived bounds.
     */
    BoundManager _boundManager;

    /*
      Collect and print various statistics.
    */
    Statistics _statistics;

    /*
      The tableau object maintains the equations, assignments and bounds.
    */
    AutoTableau _tableau;

    /*
      The existing piecewise-linear constraints.
    */
    List<PiecewiseLinearConstraint *> _plConstraints;

    /*
      The existing nonlinear constraints.
    */
    List<NonlinearConstraint *> _nlConstraints;

    /*
      Piecewise linear constraints that are currently violated.
    */
    List<PiecewiseLinearConstraint *> _violatedPlConstraints;

    /*
      A single, violated PL constraint, selected for fixing.
    */
    PiecewiseLinearConstraint *_plConstraintToFix;

    /*
      Preprocessed Query
    */
    std::unique_ptr<Query> _preprocessedQuery;

    /*
      Pivot selection strategies.
    */
    BlandsRule _blandsRule;
    DantzigsRule _dantzigsRule;
    AutoProjectedSteepestEdgeRule _projectedSteepestEdgeRule;
    EntrySelectionStrategy *_activeEntryStrategy;

    /*
      Bound tightener.
    */
    AutoRowBoundTightener _rowBoundTightener;

    /*
      The SMT engine is in charge of case splitting.
    */
    SmtCore _smtCore;

    /*
      Number of pl constraints disabled by valid splits.
    */
    unsigned _numPlConstraintsDisabledByValidSplits;

    /*
      Degradation checker.
    */
    DegradationChecker _degradationChecker;

    /*
      Query preprocessor.
    */
    Preprocessor _preprocessor;

    /*
      Is preprocessing enabled?
    */
    bool _preprocessingEnabled;

    /*
      Is the initial state stored?
    */
    bool _initialStateStored;

    /*
      Work memory (of size m)
    */
    double *_work;

    /*
      Restoration status.
    */
    BasisRestorationRequired _basisRestorationRequired;
    BasisRestorationPerformed _basisRestorationPerformed;

    /*
      Used to restore tableau precision when degradation becomes excessive.
    */
    PrecisionRestorer _precisionRestorer;

    /*
      Cost function manager.
    */
    AutoCostFunctionManager _costFunctionManager;

    /*
      Indicates a user/DnCManager request to quit
    */
    std::atomic_bool _quitRequested;

    /*
      A code indicating how the run terminated.
    */
    ExitCode _exitCode;

    /*
      The number of visited states when we performed the previous
      restoration. This field serves as an indication of whether or
      not progress has been made since the previous restoration.
    */
    unsigned long long _numVisitedStatesAtPreviousRestoration;

    /*
      An object that knows the topology of the network being checked,
      and can be used for various operations such as network
      evaluation of topology-based bound tightening.
     */
    NLR::NetworkLevelReasoner *_networkLevelReasoner;

    /*
      Verbosity level:
      0: print out minimal information
      1: print out statistics only in the beginning and the end
      2: print out statistics during solving
    */
    unsigned _verbosity;

    /*
      Records for checking whether the solution process is, overall,
      making progress. _lastNumVisitedStates stores the previous number
      of visited tree states, and _lastIterationWithProgress stores the
      last iteration number where the number of visited tree states was
      observed to increase.
    */
    unsigned _lastNumVisitedStates;
    unsigned long long _lastIterationWithProgress;

    /*
      Type of symbolic bound tightening
    */
    SymbolicBoundTighteningType _symbolicBoundTighteningType;

    /*
      Disjunction that is used for splitting but doesn't exist in the beginning
    */
    std::unique_ptr<PiecewiseLinearConstraint> _disjunctionForSplitting;

    /*
      Solve the query with MILP encoding
    */
    bool _solveWithMILP;

    /*
      The solver to solve the LP during the complete search.
    */
    LPSolverType _lpSolverType;

    /*
      GurobiWrapper object
    */
    std::unique_ptr<GurobiWrapper> _gurobi;

    /*
      MILPEncoder
    */
    std::unique_ptr<MILPEncoder> _milpEncoder;

    /*
      Manager of the SoI cost function.
    */
    std::unique_ptr<SumOfInfeasibilitiesManager> _soiManager;

    /*
      Stored options
      Do this since Options object is not thread safe and
      there is a chance that multiple Engine object be accessing the Options object.
    */
    unsigned _simulationSize;
    bool _isGurobyEnabled;
    bool _performLpTighteningAfterSplit;
    MILPSolverBoundTighteningType _milpSolverBoundTighteningType;

    /*
      SnC Split
     */
    bool _sncMode;
    PiecewiseLinearCaseSplit _sncSplit;

    /*
      Query Identifier
     */
    String _queryId;

    /*
      Frequency to print the statistics.
    */
    unsigned _statisticsPrintingFrequency;

    LinearExpression _heuristicCost;

    /*
      Perform a simplex step: compute the cost function, pick the
      entering and leaving variables and perform a pivot.
      Return true only if the current assignment is optimal
      with respect to _heuristicCost.
    */
    bool performSimplexStep();

    /*
      Perform a constraint-fixing step: select a violated piece-wise
      linear constraint and attempt to fix it.
    */
    void performConstraintFixingStep();

    /*
      Attempt to fix one of the violated pl constraints.
    */
    void fixViolatedPlConstraintIfPossible();

    /*
      Return true iff all variables are within bounds.
     */
    bool allVarsWithinBounds() const;

    /*
      Collect all violated piecewise linear constraints.
    */
    void collectViolatedPlConstraints();

    /*
      Return true iff all piecewise linear constraints hold.
    */
    bool allPlConstraintsHold();

    /*
      Return true iff all nonlinear constraints hold.
    */
    bool allNonlinearConstraintsHold();

    /*
      Return true iff there are active unfixed constraints
    */
    bool hasBranchingCandidate();

    /*
      Select a currently-violated LP constraint for fixing
    */
    void selectViolatedPlConstraint();

    /*
      Report the violated PL constraint to the SMT engine.
    */
    void reportPlViolation();

    /*
      Apply any bound tightenings found by the row tightener.
    */
    void applyAllRowTightenings();

    /*
      Apply any bound tightenings entailed by the constraints.
    */
    void applyAllConstraintTightenings();

    bool applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint );

    /*
      Update statitstics, print them if needed.
    */
    void mainLoopStatistics();

    /*
      Perform bound tightening after performing a case split.
    */
    void performBoundTighteningAfterCaseSplit();

    /*
      Called after a satisfying assignment is found for the linear constraints.
      Now we try to satisfy the piecewise linear constraints with
      "local" search (either with Reluplex-styled constraint fixing
      or SoI-based stochastic search).

      The method also has the side effect of making progress towards the
      branching condition.

      Return true iff a true satisfying assignment is found.
    */
    bool adjustAssignmentToSatisfyNonLinearConstraints();

    /*
      Perform precision restoration if needed. Return true iff precision
      restoration is performed.
    */
    bool performPrecisionRestorationIfNeeded();

    /*
      Check if the current degradation is high
    */
    bool shouldCheckDegradation();
    bool highDegradation();

    /*
      Handle malformed basis exception. Return false if unable to restore
      precision.
    */
    bool handleMalformedBasisException();

    /*
      Perform bound tightening on the constraint matrix A.
    */
    void tightenBoundsOnConstraintMatrix();

    /*
      Adjust the size of the work memory. Should be called when m changes.
    */
    void adjustWorkMemorySize();

    /*
      Store the original engine state within the precision restorer.
      Restore the tableau from the original version.
    */
    void storeInitialEngineState();
    void performPrecisionRestoration( PrecisionRestorer::RestoreBasics restoreBasics );
    bool basisRestorationNeeded() const;

    /*
      For debugging purposes:
      Check that the current lower and upper bounds are consistent
      with the stored solution
    */
    void checkBoundCompliancyWithDebugSolution();

    /*
      A helper function for merging the columns of two variables.
      This function will ensure that the variables are non-basic
      and then attempt to merge them. Returns true if successful,
      false otherwise.
    */
    bool attemptToMergeVariables( unsigned x1, unsigned x2 );

    void performDeepPolyAnalysis();

    /*
      Perform a round of symbolic bound tightening, taking into
      account the current state of the piecewise linear constraints.
    */
    unsigned performSymbolicBoundTightening( Query *inputQuery = nullptr );

    /*
      Perform a simulation which calculates concrete values of each layer with
      randomly generated input values.
    */
    void performSimulation();

    /*
      Check whether a timeout value has been provided and exceeded.
    */
    bool shouldExitDueToTimeout( double timeout ) const;

    /*
      Evaluate the network on legal inputs; obtain the assignment
      for as many intermediate nodes as possible; and then try
      to assign these values to the corresponding variables.
    */
    void warmStart();

    /*
      Check whether the number of visited tree states has increased
      recently. If not, request a precision restoration.
    */
    void checkOverallProgress();

    /*
      Helper functions for input query preprocessing
    */
    void invokePreprocessor( const IQuery &inputQuery, bool preprocess );
    void printInputBounds( const IQuery &inputQuery ) const;
    void storeEquationsInDegradationChecker();
    void removeRedundantEquations( const double *constraintMatrix );
    void selectInitialVariablesForBasis( const double *constraintMatrix,
                                         List<unsigned> &initialBasis,
                                         List<unsigned> &basicRows );
    void initializeTableau( const double *constraintMatrix, const List<unsigned> &initialBasis );
    void initializeBoundsAndConstraintWatchersInTableau( unsigned numberOfVariables );
    void initializeNetworkLevelReasoning();
    double *createConstraintMatrix();
    void addAuxiliaryVariables();
    void augmentInitialBasisIfNeeded( List<unsigned> &initialBasis,
                                      const List<unsigned> &basicRows );
    void performMILPSolverBoundedTightening( Query *inputQuery = nullptr );

    void performAdditionalBackwardAnalysisIfNeeded();

    /*
      Call MILP bound tightening for a single layer.
    */
    void performMILPSolverBoundedTighteningForSingleLayer( unsigned targetIndex );

    /*
      Update the preferred direction to perform fixes and the preferred order
      to handle case splits
    */
    void updateDirections();

    /*
      Decide which branch heuristics to use.
    */
    void decideBranchingHeuristics();

    /*
      Pick the ReLU with the highest BaBSR heuristic score.
    */
    PiecewiseLinearConstraint *pickSplitPLConstraintBasedOnBaBsrHeuristic();

    /*
      Among the earliest K ReLUs, pick the one with Polarity closest to 0.
      K is equal to GlobalConfiguration::POLARITY_CANDIDATES_THRESHOLD
    */
    PiecewiseLinearConstraint *pickSplitPLConstraintBasedOnPolarity();

    /*
      Pick the first unfixed ReLU in the topological order
    */
    PiecewiseLinearConstraint *pickSplitPLConstraintBasedOnTopology();

    /*
      Pick the input variable with the largest interval
    */
    PiecewiseLinearConstraint *pickSplitPLConstraintBasedOnIntervalWidth();

    /*
      Solve the input query with a MILP solver (Gurobi)
    */
    bool solveWithMILPEncoding( double timeoutInSeconds );

    /*
      Perform SoI-based stochastic local search
    */
    bool performDeepSoILocalSearch();

    /*
      Update the pseudo impact of the PLConstraints according to the cost of the
      phase patterns. For example, if the minimum of the last accepted phase
      pattern is 0.5, the minimum of the last proposed phase pattern is 0.2.
      And the two phase patterns differ by the cost term of a PLConstaint f.
      Then the Pseudo Impact of f is updated by |0.5 - 0.2| using exponential
      moving average.
    */
    void updatePseudoImpactWithSoICosts( double costOfLastAcceptedPhasePattern,
                                         double costOfProposedPhasePattern );

    /*
      This is called when handling the case when the SoI is already 0 but
      the PLConstraints not participating in the SoI are not satisfied.
      In that case, we bump up the score of those non-participating
      PLConstraints to promote them in the branching order.
    */
    void bumpUpPseudoImpactOfPLConstraintsNotInSoI();

    /*
      If we are using an external solver for LP solving, we need to inform
      the solver of the up-to-date variable bounds before invoking it.
    */
    void informLPSolverOfBounds();

    /*
      Minimize the given cost function with Gurobi. Return true if
      the cost function is minimized. Throw InfeasibleQueryException if
      the constraints in _gurobi are infeasible. Throw an error otherwise.
    */
    bool minimizeCostWithGurobi( const LinearExpression &costFunction );

    /*
      Get Context reference
    */
    Context &getContext()
    {
        return _context;
    }

    /*
       Checks whether the current bounds are consistent. Exposed for the SmtCore.
     */
    bool consistentBounds() const;

    /*
      DEBUG only
      Check that the variable bounds in Gurobi is up-to-date.
    */
    void checkGurobiBoundConsistency() const;

    /*
      Proof Production data structes
     */

    bool _produceUNSATProofs;
    BoundManager _groundBoundManager;
    UnsatCertificateNode *_UNSATCertificate;
    CVC4::context::CDO<UnsatCertificateNode *> *_UNSATCertificateCurrentPointer;

    /*
      Returns true iff there is a variable with bounds that can explain infeasibility of the tableau
    */
    bool certifyInfeasibility( unsigned var ) const;

    /*
      Returns the value of a variable bound, as explained by the BoundExplainer
    */
    double explainBound( unsigned var, bool isUpper ) const;

    /*
     Returns true iff both bounds are epsilon close to their explained bounds
    */
    bool validateBounds( unsigned var, double epsilon, bool isUpper ) const;

    /*
     Returns true iff all bounds are epsilon-close to their explained bounds
    */
    bool validateAllBounds( double epsilon ) const;

    /*
      Finds the variable causing failure and updates its bounds explanations
    */
    void explainSimplexFailure();

    /*
      Sanity check for ground bounds, returns true iff all bounds are at least as tight as their
      ground bounds
    */
    bool checkGroundBounds() const;

    /*
      Updates bounds after deducing Simplex infeasibility, according to a tableau row
    */
    unsigned explainFailureWithTableau();

    /*
      Updates bounds after deducing Simplex infeasibility, according to the cost function
    */
    unsigned explainFailureWithCostFunction();

    /*
      Updates an explanation of a bound according to a row, and checks for an explained
      contradiction. If a contradiction can be deduced, return true. Else, revert and return false
    */
    bool explainAndCheckContradiction( unsigned var, bool isUpper, const TableauRow *row );
    bool explainAndCheckContradiction( unsigned var, bool isUpper, const SparseUnsortedList *row );

    /*
      Delegates leaves with certification error to SMTLIB format
    */
    void markLeafToDelegate();

    /*
      Return the vector given by upper bound explanation - lower bound explanation
      Assuming infeasibleVar is indeed infeasible, then the result is a contradiction vector
     */
    const Vector<double> computeContradiction( unsigned infeasibleVar ) const;

    /*
      Writes the details of a contradiction to the UNSAT certificate node
    */
    void writeContradictionToCertificate( unsigned infeasibleVar ) const;
};

#endif // __Engine_h__
