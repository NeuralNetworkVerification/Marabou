/*********************                                                        */
/*! \file IEngine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __IEngine_h__
#define __IEngine_h__

#include "BoundExplainer.h"
#include "DivideStrategy.h"
#include "ExitCode.h"
#include "GroundBoundManager.h"
#include "LPSolverType.h"
#include "List.h"
#include "PlcLemma.h"
#include "SnCDivideStrategy.h"
#include "SymbolicBoundTighteningType.h"
#include "TableauState.h"
#include "TableauStateStorageLevel.h"
#include "Vector.h"
#include "context/context.h"

#ifdef _WIN32
#undef ERROR
#endif

class EngineState;
class Equation;
namespace NLR {
class NetworkLevelReasoner;
}
class PiecewiseLinearCaseSplit;
class SearchTreeState;
class String;
class PiecewiseLinearConstraint;
class PLCLemma;
class Query;
class UnsatCertificateNode;

class IEngine
{
public:
    virtual ~IEngine(){};

    /*
      Add equations and apply tightenings from a PL case split.
    */
    virtual void applySplit( const PiecewiseLinearCaseSplit &split ) = 0;

    /*
      Apply tighetenings implied from phase fixing of the given piecewise linear constraint;
     */
    virtual void applyPlcPhaseFixingTightenings( PiecewiseLinearConstraint &constraint ) = 0;

    /*
      Register initial SnC split
    */
    virtual void applySnCSplit( PiecewiseLinearCaseSplit split, String queryId ) = 0;
    virtual bool inSnCMode() const = 0;

    /*
      Hooks invoked before/after context push/pop to store/restore/update context independent data.
    */
    virtual void preContextPushHook() = 0;
    virtual void postContextPopHook() = 0;

    /*
      Methods for storing and restoring the state of the engine.
    */
    virtual void storeState( EngineState &state, TableauStateStorageLevel level ) const = 0;
    virtual void restoreState( const EngineState &state ) = 0;
    virtual void setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints ) = 0;

    /*
      Store the current stack of the searchTreeHandler into searchTreeState
    */
    virtual void storeSearchTreeState( SearchTreeState &searchTreeState ) = 0;

    /*
      Apply the stack to the newly created SearchTreeHandler, returns false if UNSAT is
      found in this process.
    */
    virtual bool restoreSearchTreeState( SearchTreeState &searchTreeState ) = 0;

    /*
      Required initialization before starting the solving loop.
     */
    virtual void initializeSolver() = 0;

    /*
      Solve the encoded query.
    */
    virtual bool solve( double timeoutInSeconds ) = 0;

    /*
      Methods for DnC: reset the engine state for re-use,
      get input variables.
    */
    virtual void reset() = 0;
    virtual List<unsigned> getInputVariables() const = 0;

    /*
      Pick the piecewise linear constraint for internal splitting
    */
    virtual PiecewiseLinearConstraint *pickSplitPLConstraint( DivideStrategy strategy ) = 0;

    /*
      Pick the piecewise linear constraint for SnC splitting
    */
    virtual PiecewiseLinearConstraint *pickSplitPLConstraintSnC( SnCDivideStrategy strategy ) = 0;
    /*
      Return the value of a variable bound, as expressed by the bounds explainer and the initial
      bounds
    */
    virtual double explainBound( unsigned var, bool isUpper ) const = 0;

    virtual void applyAllBoundTightenings() = 0;

    virtual bool applyAllValidConstraintCaseSplits() = 0;
    /*
      Get Context reference
     */
    virtual CVC4::context::Context &getContext() = 0;

    virtual bool consistentBounds() const = 0;

    /*
      Returns true iff the engine is in proof production mode
    */
    virtual bool shouldProduceProofs() const = 0;

    /*
      Get the ground bound of the variable
    */
    virtual double getGroundBound( unsigned var, bool isUpper ) const = 0;
    virtual std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    getGroundBoundEntry( unsigned var, bool isUpper ) const = 0;

    /*
      Get the current pointer in the UNSAT certificate node
    */
    virtual UnsatCertificateNode *getUNSATCertificateCurrentPointer() const = 0;

    /*
      Set the current pointer in the UNSAT certificate
    */
    virtual void setUNSATCertificateCurrentPointer( UnsatCertificateNode *node ) = 0;

    /*
      Get the root of the UNSAT certificate proof tree
    */
    virtual const UnsatCertificateNode *getUNSATCertificateRoot() const = 0;

    /*
      Certify the UNSAT certificate
    */
    virtual bool certifyUNSATCertificate() = 0;

    /*
      Finds the variable causing failure and updates its bounds explanations
    */
    virtual void explainSimplexFailure() = 0;

    /*
      Get the boundExplainer
    */
    virtual const BoundExplainer *getBoundExplainer() const = 0;

    /*
      Set the boundExplainer content
    */
    virtual void setBoundExplainerContent( BoundExplainer *boundExplainer ) = 0;

    /*
      Propagate bound tightenings stored in the BoundManager
    */
    virtual bool propagateBoundManagerTightenings() = 0;

    /*
      Add lemma to the UNSAT Certificate
    */
    virtual void incNumOfLemmas() = 0;

    /*
      Add ground bound entry using a lemma
    */
    virtual std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    setGroundBoundFromLemma( const std::shared_ptr<PLCLemma> lemma, bool isPhaseFixing ) = 0;

    /*
      Returns true if the query should be solved using MILP
     */
    virtual bool shouldSolveWithMILP() const = 0;

    virtual void assertEngineBoundsForSplit( const PiecewiseLinearCaseSplit &split ) = 0;

    /*
      Check whether a timeout value has been provided and exceeded.
    */
    virtual bool shouldExitDueToTimeout( double timeout ) const = 0;

    /*
      Returns the verbosity level.
    */
    virtual unsigned getVerbosity() const = 0;

    virtual void exportQueryWithError( String ) = 0;

    /*
      Returns the exit code.
    */
    virtual ExitCode getExitCode() const = 0;

    /*
      Sets the exit code.
    */
    virtual void setExitCode( ExitCode exitCode ) = 0;

    /*
      Return the piecewise linear constraints list of the engine
    */
    virtual const List<PiecewiseLinearConstraint *> *getPiecewiseLinearConstraints() const = 0;

    /*
      Returns the type of the LP Solver in use.
     */
    virtual LPSolverType getLpSolverType() const = 0;

    /*
      Returns a pointer to the internal NLR object.
     */
    virtual NLR::NetworkLevelReasoner *getNetworkLevelReasoner() const = 0;

    virtual void restoreInitialEngineState() = 0;

    /*
     Solve the input query with a MILP solver
    */
    virtual bool solveWithMILPEncoding( double timeoutInSeconds ) = 0;

    /*
     Should solve the input query with CDCL?
    */
    virtual bool shouldSolveWithCDCL() const = 0;

    /*
      Return the output variables.
     */
    virtual List<unsigned> getOutputVariables() const = 0;

    /*
     Returns the symbolic bound tightening type in use.
    */
    virtual SymbolicBoundTighteningType getSymbolicBoundTighteningType() const = 0;

    /*
      Returns the bound manager
     */
    virtual const IBoundManager *getBoundManager() const = 0;

    /*
      Returns the input query.
     */
    virtual std::shared_ptr<Query> getInputQuery() const = 0;

#ifdef BUILD_CADICAL
    /*
      Solve the input query with CDCL
    */
    virtual bool solveWithCDCL( double timeoutInSeconds ) = 0;

    /*
      Methods for creating conflict clauses and lemmas, for CDCL.
     */
    virtual Set<int> clauseFromContradictionVector( const SparseUnsortedList &explanation,
                                                    unsigned id,
                                                    int explainedVar,
                                                    bool isUpper,
                                                    double targetBound ) = 0;
    virtual Set<int> explainPhaseWithProof( const PiecewiseLinearConstraint *litConstraint ) = 0;

    /*
      Explain infeasibility of Gurobi, for CDCL conflict clauses
    */
    virtual void explainGurobiFailure() = 0;

    /*
      Returns true if the current assignment complies with the given clause (CDCL).
     */
    virtual bool checkAssignmentComplianceWithClause( const Set<int> &clause ) const = 0;

    /*
      Remove a literal from the propagation list to the SAT solver, during the CDCL solving.
     */
    virtual void removeLiteralFromPropagations( int literal ) = 0;
#endif
};

#endif // __IEngine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
