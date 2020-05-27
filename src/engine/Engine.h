/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Engine_h__
#define __Engine_h__

#include "AutoConstraintBoundTightener.h"
#include "AutoCostFunctionManager.h"
#include "AutoProjectedSteepestEdge.h"
#include "AutoRowBoundTightener.h"
#include "AutoTableau.h"
#include "BlandsRule.h"
#include "DantzigsRule.h"
#include "DegradationChecker.h"
#include "DivideStrategy.h"
#include "IEngine.h"
#include "InputQuery.h"
#include "Map.h"
#include "PrecisionRestorer.h"
#include "Preprocessor.h"
#include "SignalHandler.h"
#include "SmtCore.h"
#include "Statistics.h"

#include <atomic>

#ifdef _WIN32
#undef ERROR
#endif

class EngineState;
class InputQuery;
class PiecewiseLinearConstraint;
class String;

class Engine : public IEngine, public SignalHandler::Signalable
{
public:
    Engine( unsigned verbosity = 2 );
    ~Engine();

    /*
      Attempt to find a feasible solution for the input within a time limit
      (a timeout of 0 means no time limit). Returns true if found, false if infeasible.
    */
    bool solve( unsigned timeoutInSeconds = 0 );

    /*
      Process the input query and pass the needed information to the
      underlying tableau. Return false if query is found to be infeasible,
      true otherwise.
     */
    bool processInputQuery( InputQuery &inputQuery );
    bool processInputQuery( InputQuery &inputQuery, bool preprocess );

    /*
      If the query is feasiable and has been successfully solved, this
      method can be used to extract the solution.
     */
    void extractSolution( InputQuery &inputQuery );

    /*
      Methods for storing and restoring the state of the engine.
    */
    void storeState( EngineState &state, bool storeAlsoTableauState ) const;
    void restoreState( const EngineState &state );
    void setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints );

    /*
      A request from the user to terminate
    */
    void quitSignal();

    const Statistics *getStatistics() const;

    InputQuery *getInputQuery();

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
      Pick the piecewise linear constraint for splitting
    */
    PiecewiseLinearConstraint *pickSplitPLConstraint();

    /*
      Update the scores of each candidate splitting PL constraints
    */
    void updateScores();

    /*
      Set the constraint violation threshold of SmtCore
    */
    void setConstraintViolationThreshold( unsigned threshold );

    /*
      PSA: The following two methods are for DnC only and should be used very
      cautiously.
     */
    void resetSmtCore();
    void resetExitCode();
    void resetBoundTighteners();

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
      The ordered set of candidate PL constraints for splitting
    */
    Set<PiecewiseLinearConstraint *> _candidatePlConstraints;

    /*
      Piecewise linear constraints that are currently violated.
    */
    List<PiecewiseLinearConstraint *> _violatedPlConstraints;

    /*
      A single, violated PL constraint, selected for fixing.
    */
    PiecewiseLinearConstraint *_plConstraintToFix;

	/*
	  Preprocessed InputQuery
	*/
	InputQuery _preprocessedQuery;

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
      An object in charge of managing bound tightenings
      proposed by the PiecewiseLinearConstriants.
    */
    AutoConstraintBoundTightener _constraintBoundTightener;

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
    NetworkLevelReasoner *_networkLevelReasoner;

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
      Perform a simplex step: compute the cost function, pick the
      entering and leaving variables and perform a pivot.
    */
    void performSimplexStep();

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
      Select a currently-violated LP constraint for fixing
    */
    void selectViolatedPlConstraint();

    /*
      Report the violated PL constraint to the SMT engine.
    */
    void reportPlViolation();

    /*
      Apply all bound tightenings (row and matrix-based) in
      the queue.
    */
    void applyAllBoundTightenings();

    /*
      Apply any bound tightenings found by the row tightener.
    */
    void applyAllRowTightenings();

    /*
      Apply any bound tightenings entailed by the constraints.
    */
    void applyAllConstraintTightenings();

    /*
      Apply all valid case splits proposed by the constraints.
      Return true if a valid case split has been applied.
    */
    bool applyAllValidConstraintCaseSplits();
    bool applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint );

    /*
      Update statitstics, print them if needed.
    */
    void mainLoopStatistics();

    /*
      Check if the current degradation is high
    */
    bool shouldCheckDegradation();
    bool highDegradation();

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

    static void log( const String &message );

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

    /*
      Perform a round of symbolic bound tightening, taking into
      account the current state of the piecewise linear constraints.
    */
    void performSymbolicBoundTightening();

    /*
      Check whether a timeout value has been provided and exceeded.
    */
    bool shouldExitDueToTimeout( unsigned timeout ) const;

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
    void informConstraintsOfInitialBounds( InputQuery &inputQuery ) const;
    void invokePreprocessor( const InputQuery &inputQuery, bool preprocess );
    void printInputBounds( const InputQuery &inputQuery ) const;
    void storeEquationsInDegradationChecker();
    void removeRedundantEquations( const double *constraintMatrix );
    void selectInitialVariablesForBasis( const double *constraintMatrix, List<unsigned> &initialBasis, List<unsigned> &basicRows );
    void initializeTableau( const double *constraintMatrix, const List<unsigned> &initialBasis );
    void initializeNetworkLevelReasoning();
    double *createConstraintMatrix();
    void addAuxiliaryVariables();
    void augmentInitialBasisIfNeeded( List<unsigned> &initialBasis, const List<unsigned> &basicRows );

    /*
      Update the preferred direction to perform fixes and the preferred order
      to handle case splits
    */
    void updateDirections();
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
