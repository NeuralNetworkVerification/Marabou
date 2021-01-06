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
 ** context-dependent (CD) data structures from CVC4, which lazily and automatically
 ** backtrack in sync with the central context object. These data structures
 ** are delicate and require that data stored is safe to memcopy around.
 **
 **
 ** Members are initialized using the initializeCDOs( Context context ) method.
 ** Subclasses need to call initializeDuplicatesCDOs() method in the
 ** duplicateConstraint() method to avoid sharing context-dependent members.
 ** CDOs need not be initialized for pre-processing purposes.
 **
 ** The class uses CDOs to record:
 **  * _cdConstraintActive
 **  * _cdPhaseStatus
 **  * _cdInfeasibleCases
 **
 ** PhaseStatus enumeration is used to keep track of current phase of the
 ** object, as well as to record previously explored phases. This replaces
 ** moving PiecewiseLinearCaseSplits around.
 **
 ** ContextDependentPiecewiseLinearConstraint stores locally which phases have
 ** been explored so far using _cdInfeasibleCases. To communicate with the
 ** search it implements two methods:
 **   * markInfeasible( PhaseStatus caseId ) - marks explored phase
 **   * nextFeasibleCase() - obtains next unexplored case if one exists
 **
 ** These methods rely on subclass properly handling:
 **  * _numCases field, which denotes the number of piecewise linear
 **     segments the constraint has, initialized by the constructor.
 **  * getAllCases() virtual method returns all possible phases represented using
 **    PhaseStatus enumeration. The order of returned cases will determine the
 **    search order, so this method should implement any search heuristics.
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

    virtual ~ContextDependentPiecewiseLinearConstraint()
    {
        cdoCleanup();
    }

    /*
      Turn the constraint on/off.
    */
    void setActiveConstraint( bool active );
    bool isActive() const;

    /*
       Returns a list of all cases of this constraint and is used by the
       nextFeasibleCase during search. The order of returned cases affects the
       search, and this method is where related heuristics should be
       implemented.
     */
    virtual List<PhaseStatus> getAllCases() const = 0;

    /*
     * Returns case split corresponding to the given phase/id
       TODO: Update the signature in PiecewiseLinearConstraint, once the new
       search is integrated.
     */
    virtual PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus caseId ) const = 0;


    /*
      Check if the constraint's phase has been fixed.
    */
    virtual bool phaseFixed() const = 0;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    virtual PiecewiseLinearCaseSplit getImpliedCaseSplit() const = 0;

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

    /*
      Get the context object - debugging purposes only
    */
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
      Get the infeasible cases object - debugging purposes only
    */
    CVC4::context::CDList<PhaseStatus> *getInfeasibleCasesCDList() const
    {
        return _cdInfeasibleCases;
    }

    /*
       Mark that an exploredCase is infeasible, reducing the remaining search space.
     */
    void markInfeasible( PhaseStatus exploredCase );

    /*
      Returns phaseStatus representing next feasible case.
      Returns PHASE_NOT_FIXED if no feasible case exists.
      Assumes the contraint phase is PHASE_NOT_FIXED.
      Worst case complexity O(n^2)
      This method should be overloaded for MAX and DISJUNCTION constraints.
     */
    PhaseStatus nextFeasibleCase();

    /*
       Returns number of cases not yet marked as infeasible.
     */
    unsigned numFeasibleCases()
    {
        return _numCases - _cdInfeasibleCases->size();
    }

    /*
        Returns true if there are feasible cases remaining.
     */
    bool isFeasible()
    {
        return numFeasibleCases() > 0u;
    }

    /*
       Returns true if there is only one feasible case remaining.
     */
    bool isImplication()
    {
        return 1u == numFeasibleCases();
    }

protected:
    unsigned _numCases; // Number of possible cases/phases for this constraint
                        // (e.g. 2 for ReLU, ABS, SIGN; >=2 for Max and Disjunction )

    BoundManager *_boundManager; // Pointer to a centralized object to store bounds.
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

    /*
       Method provided to allow safe copying of the context-dependent members,
       which will be freshly initialized in a copy and with the same values.
     */
    void initializeDuplicatesCDOs( ContextDependentPiecewiseLinearConstraint *clone ) const;

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
