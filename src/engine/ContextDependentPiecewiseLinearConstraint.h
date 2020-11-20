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
 ** This class is an implementation of PiecewiseLinearConstraint using
 ** context-dependent data structures from CVC4, which lazily and automatically
 ** back-track in sync with the central context object. These data structures
 ** are delicate and require that data stored is safe to memcopy around, but
 ** provide built-in backtracking which is cheap in terms of computational overhead.
 **
 ** The basic members of PiecewiseLinearConstraint class use context-dependent
 ** objects (CDOs), which need to be initialized for the backtracking search in
 ** Marabou and are not needed for pre-processing purposes. The initialization
 ** is performed using the initializeCDOs( Context context ) method. Descendant
 ** classes need to call initializeDuplicatesCDOs() method in
 ** duplicateConstraint method to avoid sharing context-dependent members. The
 ** duplication functionality might be unnecessary (and perhaps even prohibited)
 ** after CDOs are initialized.
 **
 ** The methods used by the Marabou search:
 **  * markInfeasible( PhaseStatus caseId ) - which denotes explored search space
 **  * nextFeasibleCase() - which obtains next unexplored case if one exists
 **
 ** These methods rely on:
 **  * _numCases field, which denotes the number of piecewise linear
 **     segments the constraint has, passed through the constructor.
 **  * getAllCases() virtual method returns all possible cases represented using
 **    PhaseStatus enumeration. The order of returned cases will determine the
 **    search order, so it should implement any search heuristics.
 **/

#ifndef __ContextDependentPiecewiseLinearConstraint_h__
#define __ContextDependentPiecewiseLinearConstraint_h__

#include "FloatUtils.h"
#include "ITableau.h"
#include "List.h"
#include "Map.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "PiecewiseLinearFunctionType.h"
#include "Queue.h"
#include "Tightening.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "context/context.h"

class Equation;
class BoundManager;
class ITableau;
class InputQuery;
class String;

class ContextDependentPiecewiseLinearConstraint
   : public virtual PiecewiseLinearConstraint
{
public:
    ContextDependentPiecewiseLinearConstraint();
    ContextDependentPiecewiseLinearConstraint( unsigned numCases );
    virtual ~ContextDependentPiecewiseLinearConstraint() { cdoCleanup(); }

    /*
      Turn the constraint on/off.
    */
    void setActiveConstraint( bool active );
    bool isActive() const;

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
       Register context object. Necessary for lazy backtracking features - such
       as _cdPhaseStatus and _activeStatus. Does not require initialization until
       after pre-processing.
     */
    void initializeCDOs( CVC4::context::Context *context );

    /*
       Politely clean up allocated CDOs.
     */
    void cdoCleanup();

    CVC4::context::Context *getContext() const { return _context; }

    /*
      Get the active status object - debugging purposes only
    */
    CVC4::context::CDO<bool> *getActiveStatusCDO() const { return _cdConstraintActive; };

    /*
      Get the current phase status object - debugging purposes only
    */
    CVC4::context::CDO<PhaseStatus> *getPhaseStatusCDO() const { return _cdPhaseStatus; }

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
    unsigned _numCases;
    PhaseStatus _phaseStatus; //TODO: Move to PiecewiseLinearConstraint, before integration.
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

    /*
      Initialize CDOs.
    */
    void initializeCDActiveStatus();
    void initializeCDPhaseStatus();
    void initializeCDInfeasibleCases();
    void
    initializeDuplicatesCDOs( ContextDependentPiecewiseLinearConstraint *clone ) const;

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
