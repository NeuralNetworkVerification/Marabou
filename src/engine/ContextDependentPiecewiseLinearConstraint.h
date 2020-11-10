/*********************                                                        */
/*! \file ContextDependentPiecewiseLinearConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A context dependent implementation of PiecewiseLinearConstraint class
**/

#ifndef __ContextDependentPiecewiseLinearConstraint_h__
#define __ContextDependentPiecewiseLinearConstraint_h__

#include "context/context.h"
#include "context/cdo.h"
#include "context/cdlist.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "List.h"
#include "Map.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearFunctionType.h"
#include "Queue.h"
#include "Tightening.h"

class Equation;
class BoundManager;
class ITableau;
class InputQuery;
class String;

enum PhaseStatus : unsigned {
        PHASE_NOT_FIXED = 0,
        RELU_PHASE_ACTIVE = 1,
        RELU_PHASE_INACTIVE = 2,
        ABS_PHASE_POSITIVE = 3,
        ABS_PHASE_NEGATIVE = 4,
        };

virtual class ContextDependentPiecewiseLinearConstraint : public virtual PiecewiseLinearConstraint, public ITableau::VariableWatcher
{
public:

    ContextDependentPiecewiseLinearConstraint();
    ContextDependentPiecewiseLinearConstraint( unsigned numCases );
    virtual ~ContextDependentPiecewiseLinearConstraint()
    {
        cdoCleanup();
    }

    bool operator<( const ContextDependentPiecewiseLinearConstraint &other ) const
    {
        return _score < other._score;
    }

    /*
      Get the type of this constraint.
    */
    virtual PiecewiseLinearFunctionType getType() const = 0;

    /*
      Return a clone of the constraint. Allocates CDOs for the copy.
    */
    virtual ContextDependentPiecewiseLinearConstraint *duplicateConstraint() const = 0;

    /*
      Restore the state of this constraint from the given one.
      We have this function in order to take advantage of the polymorphically
      correct assignment operator.
    */
    virtual void restoreState( const ContextDependentPiecewiseLinearConstraint *state ) = 0;

    /*
      Register/unregister the constraint with a tableau.
    */
    virtual void registerAsWatcher( ITableau *tableau ) = 0;
    virtual void unregisterAsWatcher( ITableau *tableau ) = 0;

    /*
      The variable watcher notifcation callbacks, about a change in a variable's value or bounds.
    */
    virtual void notifyVariableValue( unsigned /* variable */, double /* value */ ) {}
    virtual void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    virtual void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}

    /*
      Turn the constraint on/off.
    */
    void setActiveConstraint( bool active )
    {
        if ( nullptr != _cdConstraintActive )
            *_cdConstraintActive = active;
        else
          _constraintActive = active;
    }

    bool isActive() const
    {
        if ( nullptr != _cdConstraintActive )
            return *_cdConstraintActive;
        else
          return _constraintActive;
    }

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint.
    */
    virtual bool participatingVariable( unsigned variable ) const = 0;

    /*
      Get the list of variables participating in this constraint.
    */
    virtual List<unsigned> getParticipatingVariables() const = 0;

    /*
      Returns true iff the assignment satisfies the constraint.
    */
    virtual bool satisfied() const = 0;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    virtual List<ContextDependentPiecewiseLinearConstraint::Fix> getPossibleFixes() const = 0;

    /*
      Return a list of smart fixes for violated constraint.
    */
    virtual List<ContextDependentPiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const = 0;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
    */
    virtual List<PiecewiseLinearCaseSplit> getCaseSplits() const = 0;

    /*
     * Returns a list of all cases of this constraint
     */
    virtual List<PhaseStatus> getAllCases() const = 0;

    /*
     * Returns case split corresponding to the given phase/id
     */
    virtual PiecewiseLinearCaseSplit getCaseSplit( unsigned caseId ) const = 0;

    /*
      Check if the constraint's phase has been fixed.
    */
    virtual bool phaseFixed() const = 0;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    virtual PiecewiseLinearCaseSplit getImpliedCaseSplit() const = 0;

    /*
      Dump the current state of the constraint.
    */
    void dump() const;

    /*
      Dump the current state of the constraint.
    */
    virtual void dump( String & ) const {}

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2). constraintObsolete() returns true iff and the constraint has become obsolote
      as a result of variable eliminations.
    */
    virtual void eliminateVariable( unsigned variable, double fixedValue ) = 0;
    virtual void updateVariableIndex( unsigned oldIndex, unsigned newIndex ) = 0;
    virtual bool constraintObsolete() const = 0;

    /*
      Get the tightenings entailed by the constraint.
    */
    virtual void getEntailedTightenings( List<Tightening> &tightenings ) const = 0;

    void setStatistics( Statistics *statistics );

    /*
      For preprocessing: get any auxiliary equations that this constraint would
      like to add to the equation pool.
    */
    virtual void addAuxiliaryEquations( InputQuery &/* inputQuery */ ) {}

    /*
      Ask the piecewise linear constraint to contribute a component to the cost
      function. If implemented, this component should be empty when the constraint is
      satisfied or inactive, and should be non-empty otherwise. Minimizing the returned
      equation should then lead to the constraint being "closer to satisfied".
    */
    virtual void getCostFunctionComponent( Map<unsigned, double> &/* cost */ ) const {}

    /*
      Produce string representation of the piecewise linear constraint.
      This representation contains only the information necessary to reproduce it
      but does not account for state or change in state during execution. Additionally
      the first string before a comma has the contraint type identifier
      (ie. "relu", "max", etc)
    */
    virtual String serializeToString() const = 0;

    /*
      Register a bound manager. If a bound manager is registered,
      this piecewise linear constraint will inform the tightener whenever
      it discovers a tighter (entailed) bound.
    */
    void registerBoundManager( BoundManager *boundManager );

    /*
      Return true if and only if this piecewise linear constraint supports
      symbolic bound tightening.
    */
    virtual bool supportsSymbolicBoundTightening() const
    {
        return false;
    }

    /*
      Return true if and only if this piecewise linear constraint supports
      the polarity metric
    */
    virtual bool supportPolarity() const
    {
        return false;
    }

    /*
      Update the preferred direction to take first when splitting on this PLConstraint
    */
    virtual void updateDirection()
    {
    }

    virtual void updateScore()
    {
    }

    /*
       Register context object. Necessary for lazy backtracking features - such
       as _cdPhaseStatus and _activeStatus. Does not require initialization until
       after pre-processing.
     */
    void initializeCDOs( CVC4::context::Context *context );

    /*
       Politely clean up allocated CDOs.
     */
    void cdoCleanup();

    CVC4::context::Context *getContext() const
    {
        return _context;
    }

    /*
      Get the active status object - debugging purposes only
    */
    CVC4::context::CDO<bool> *getActiveStatusCDO() const
    {
        return _cdConstraintActive;
    };

    /*
      Get the current phase status object - debugging purposes only
    */
    CVC4::context::CDO<PhaseStatus> *getPhaseStatusCDO() const
    {
            return _cdPhaseStatus;
    }

    /*
      Get the infeasable cases object - debugging purposes only
    */
    CVC4::context::CDList<PhaseStatus> *getInfeasibleCasesCDList() const
    {
        return _cdInfeasibleCases;
    }

    /*
       Mark that exploredCase is infeasible.
     */
    void markInfeasible( PhaseStatus exploredCase );

    /*
      Retrieve next feasible case; Worst case O(n^2)
      Returns PhaseStatus representing next feasible case.
      Returns PHASE_NOT_FIXED if no feasible case exists.
     */
    PhaseStatus nextFeasibleCase();

    bool isFeasible();
    bool isImplication();
    unsigned numFeasibleCases();

protected:
    BoundManager *_boundManager;
    CVC4::context::Context *_context;
    CVC4::context::CDO<bool> *_cdConstraintActive;

    /* ReluConstraint and AbsoluteValueConstraint use PhaseStatus enumeration.
       MaxConstraint and Disjunction interpret the PhaseStatus value as the case
       number (counts from 1, value 0 is reserved and used as PHASE_NOT_FIXED).
    */
    CVC4::context::CDO<PhaseStatus> *_cdPhaseStatus;

    /*
      Store infeasible cases under the current trail. Backtracks with context.
    */
    CVC4::context::CDList<PhaseStatus> *_cdInfeasibleCases;

    unsigned _numCases;

    /*
      Initialize CDOs.
    */
    void initializeCDActiveStatus();
    void initializeCDPhaseStatus();
    void initializeDuplicatesCDOs( ContextDependentPiecewiseLinearConstraint *clone ) const;
    void initializeCDInfeasibleCases();

    void setPhaseStatus( PhaseStatus phaseStatus );
    PhaseStatus getPhaseStatus() const;

};

#endif // __ContextDependentPiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
