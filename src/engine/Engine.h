/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
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
#include "IEngine.h"
#include "InputQuery.h"
#include "Map.h"
#include "PrecisionRestorer.h"
#include "Preprocessor.h"
#include "SignalHandler.h"
#include "SmtCore.h"
#include "Statistics.h"

class EngineState;
class InputQuery;
class PiecewiseLinearConstraint;
class String;

class Engine : public IEngine, public SignalHandler::Signalable
{
public:
    Engine();
    ~Engine();

    enum ExitCode {
        UNSAT = 0,
        SAT = 1,
        ERROR = 2,
        TIMEOUT = 3,

        NOT_DONE = 999,
    };

    /*
      Attempt to find a feasible solution for the input. Returns true
      if found, false if infeasible.
    */
    bool solve();

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

    /*
      Get the exit code
    */
    Engine::ExitCode getExitCode() const;

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
      Add equations and tightenings from a split.
    */
    void applySplit( const PiecewiseLinearCaseSplit &split );

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
      Piecewise linear constraints that are currently violated.
    */
    List<PiecewiseLinearConstraint *>_violatedPlConstraints;

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
      Indicates a user request to quit
    */
    bool _quitRequested;

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
    */
    void applyAllValidConstraintCaseSplits();
    void applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint );

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
};

#endif // __Engine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
