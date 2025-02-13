/*********************                                                        */
/*! \file PiecewiseLinearConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Aleksandar Zeljic, Derek Huang, Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
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
 ** Subclasses need to call initializeDuplicateCDOs() method in the
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
 ** PiecewiseLinearConstraint stores locally which phases have
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

#ifndef __PiecewiseLinearConstraint_h__
#define __PiecewiseLinearConstraint_h__

#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "IBoundManager.h"
#include "ITableau.h"
#include "LinearExpression.h"
#include "List.h"
#include "Map.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearFunctionType.h"
#include "Queue.h"
#include "Tightening.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "context/context.h"

class Equation;
class BoundManager;
class ITableau;
class Query;
class String;

#define TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT 2u

enum PhaseStatus : unsigned {
    PHASE_NOT_FIXED = 0,
    RELU_PHASE_ACTIVE = 1,
    RELU_PHASE_INACTIVE = 2,
    ABS_PHASE_POSITIVE = 3,
    ABS_PHASE_NEGATIVE = 4,
    SIGN_PHASE_POSITIVE = 5,
    SIGN_PHASE_NEGATIVE = 6,

    // SPECIAL VALUE FOR ELIMINATED MAX CASES
    MAX_PHASE_ELIMINATED = 999999,
    // SENTINEL VALUE
    CONSTRAINT_INFEASIBLE = 1000000,
};

class PiecewiseLinearConstraint : public ITableau::VariableWatcher
{
public:
    /*
      A possible fix for a violated piecewise linear constraint: a
      variable whose value should be changed.
    */
    struct Fix
    {
    public:
        Fix( unsigned variable, double value )
            : _variable( variable )
            , _value( value )
        {
        }

        bool operator==( const Fix &other ) const
        {
            return _variable == other._variable && FloatUtils::areEqual( _value, other._value );
        }

        unsigned _variable;
        double _value;
    };

    PiecewiseLinearConstraint();
    PiecewiseLinearConstraint( unsigned numCases );
    virtual ~PiecewiseLinearConstraint()
    {
        cdoCleanup();
    }

    bool operator<( const PiecewiseLinearConstraint &other ) const
    {
        return _score < other._score;
    }

    /*
      Get the type of this constraint.
    */
    virtual PiecewiseLinearFunctionType getType() const = 0;

    /*
      Return a clone of the constraint.
    */
    virtual PiecewiseLinearConstraint *duplicateConstraint() const = 0;

    /*
      Restore the state of this constraint from the given one.
      We have this function in order to take advantage of the polymorphically
      correct assignment operator.
    */
    virtual void restoreState( const PiecewiseLinearConstraint *state ) = 0;

    /*
      Register/unregister the constraint with a talbeau.
    */
    virtual void registerAsWatcher( ITableau *tableau ) = 0;
    virtual void unregisterAsWatcher( ITableau *tableau ) = 0;

    /*
      The variable watcher notifcation callbacks, about a change in a variable's value or bounds.
    */
    virtual void notifyLowerBound( unsigned /* variable */, double /* bound */ )
    {
    }
    virtual void notifyUpperBound( unsigned /* variable */, double /* bound */ )
    {
    }

    /*
      Turn the constraint on/off.
    */
    virtual void setActiveConstraint( bool active );
    virtual bool isActive() const;

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
    virtual List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const = 0;

    /*
      Return a list of smart fixes for violated constraint.
    */
    virtual List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const = 0;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
    */
    virtual List<PiecewiseLinearCaseSplit> getCaseSplits() const = 0;

    /*
      Check if the constraint's phase has been fixed.
    */
    virtual bool phaseFixed() const = 0;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
      Transitioning from Valid to Implied with integration of
      context-dependentSMTCore.
    */
    virtual PiecewiseLinearCaseSplit getValidCaseSplit() const = 0;
    virtual PiecewiseLinearCaseSplit getImpliedCaseSplit() const = 0;

    /*
       Returns a list of all cases of this constraint. Used by the
       nextFeasibleCase to track the state during search. The order of returned
       cases affects the search, and this method is where related heuristics
       should be implemented.
     */
    virtual List<PhaseStatus> getAllCases() const = 0;

    /*
       Returns case split corresponding to the given case
     */
    virtual PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus caseId ) const = 0;

    /*
      Dump the current state of the constraint.
    */
    virtual void dump( String & ) const
    {
    }

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

    /*
      Transform the piecewise linear constraint so that each disjunct contains
      only bound constraints.
    */
    virtual void transformToUseAuxVariables( Query & ){};

    void setStatistics( Statistics *statistics );

    /*
      Before solving: get additional auxiliary euqations (typically bound-dependent)
      that this constraint would like to add to the equation pool.
    */
    virtual void addAuxiliaryEquationsAfterPreprocessing( Query & /* inputQuery */ )
    {
    }

    /*
      Whether the constraint can contribute the SoI cost function.
    */
    virtual bool supportSoI() const
    {
        return false;
    };

    virtual bool supportVariableElimination() const
    {
        return true;
    };

    /*
      Ask the piecewise linear constraint to add its cost term corresponding to
      the given phase to the cost function.
      Nothing should be added when the constraint is fixed or inactive.
      Minimizing the added term should lead to the constraint being
      "closer to satisfied" in the given phase status.
    */
    virtual void getCostFunctionComponent( LinearExpression & /* cost */,
                                           PhaseStatus /* phase */ ) const
    {
    }

    /*
      Return the phase status corresponding to the values of the input
      variables in the given assignment. For instance, for ReLU, if the input
      variable's assignment is positive, then the method returns
      RELU_PHASE_ACTIVE. Otherwise, it returns RELU_PHASE_INACTIVE.
    */
    virtual PhaseStatus
    getPhaseStatusInAssignment( const Map<unsigned, double> & /* assignment */ ) const
    {
        throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
    }

    /*
      Produce string representation of the piecewise linear constraint.
      This representation contains only the information necessary to reproduce it
      but does not account for state or change in state during execution. Additionally
      the first string before a comma has the contraint type identifier
      (ie. "relu", "max", etc)
    */
    virtual String serializeToString() const = 0;

    /*
      Return true if and only if this piecewise linear constraint supports
      the polarity metric
    */
    virtual bool supportPolarity() const
    {
        return false;
    }

    /*
      Return true if and only if this piecewise linear constraint supports the BaBsr Heuristic
    */
    virtual bool supportBaBsr() const
    {
        return false;
    }

    /*
      Update the preferred direction to take first when splitting on this PLConstraint
    */
    virtual void updateDirection()
    {
    }

    double getScore() const
    {
        return _score;
    }

    virtual void updateScoreBasedOnBaBsr()
    {
    }

    virtual void updateScoreBasedOnPolarity()
    {
    }

    /*
      Update _score with score
    */
    void setScore( double score )
    {
        _score = score;
    }

    /*
      Register the GurobiWrapper object. We will query it for assignment.
    */
    inline void registerGurobi( GurobiWrapper *gurobi )
    {
        _gurobi = gurobi;
    }

    inline void registerTableau( ITableau *tableau )
    {
        _tableau = tableau;
    }
    /*
      Method to set PhaseStatus of the constraint. Encapsulates both context
      dependent and context-less behavior. Initialized to PHASE_NOT_FIXED.
     */
    void setPhaseStatus( PhaseStatus phaseStatus );

    /*
      Method to get PhaseStatus of the constraint. Encapsulates both context
      dependent and context-less behavior.
    */
    PhaseStatus getPhaseStatus() const;

    /**********************************************************************/
    /*          Context-dependent Members Initialization and Cleanup      */
    /**********************************************************************/

    /*
      Register a bound manager. If a bound manager is registered,
      the piecewise linear constraint will inform the manager whenever
      it discovers a tighter (entailed) bound.
    */
    void registerBoundManager( IBoundManager *boundManager );

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

    /**********************************************************************/
    /*             Context-dependent Search State Interface               */
    /**********************************************************************/

    /*
       Mark that an exploredCase is infeasible, reducing the remaining search space.
     */
    void markInfeasible( PhaseStatus exploredCase );

    /*
      Returns phaseStatus representing next feasible case.
      Returns CONSTRAINT_INFEASIBLE if no feasible case exists.
      Assumes the contraint phase is PHASE_NOT_FIXED.
      Worst case complexity O(n^2)
      This method is overloaded in MAX and DISJUNCTION constraints.
     */
    virtual PhaseStatus nextFeasibleCase();

    /*
       Returns number of cases not yet marked as infeasible.
     */
    unsigned numFeasibleCases() const
    {
        return _numCases - _cdInfeasibleCases->size();
    }

    /*
        Returns true if there are feasible cases remaining.
     */
    bool isFeasible() const
    {
        return numFeasibleCases() > 0u;
    }

    /*
       Returns true if there is only one feasible case remaining.
     */
    virtual bool isImplication() const
    {
        return numFeasibleCases() == 1u;
    }

    /**********************************************************************/
    /*                       Debugging helper methods                     */
    /**********************************************************************/
    /* NOTE: Should properly live in a test extension of the class */

    /*
      Get the context object - debugging purposes only
    */
    const CVC4::context::Context *getContext() const
    {
        return _context;
    }

    /*
      Get the active status object - debugging purposes only
    */
    const CVC4::context::CDO<bool> *getActiveStatusCDO() const
    {
        return _cdConstraintActive;
    };

    /*
      Get the current phase status object - debugging purposes only
    */
    const CVC4::context::CDO<PhaseStatus> *getPhaseStatusCDO() const
    {
        return _cdPhaseStatus;
    }

    /*
      Get the infeasible cases object - debugging purposes only
    */
    const CVC4::context::CDList<PhaseStatus> *getInfeasibleCasesCDList() const
    {
        return _cdInfeasibleCases;
    }

    /*
      Add a variable to the list of aux vars designated in the Tableau, add connect it to the
      constraintAuxVariable
    */
    virtual void addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar ) = 0;

    /*
      Get the native auxiliary vars
    */
    virtual const List<unsigned> getNativeAuxVars() const
    {
        return {};
    }

    /*
      Get the tableau auxiliary vars
    */
    virtual const List<unsigned> &getTableauAuxVars() const
    {
        return _tableauAuxVars;
    }

protected:
    unsigned _numCases; // Number of possible cases/phases for this constraint
                        // (e.g. 2 for ReLU, ABS, SIGN; >=2 for Max and Disjunction )
    bool _constraintActive;
    PhaseStatus _phaseStatus;
    Map<unsigned, double> _assignment;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;

    IBoundManager *_boundManager; // Pointer to a centralized object to store bounds.
    ITableau *_tableau; // Pointer to tableau which simulates CBT until we switch to CDSmtCore

    CVC4::context::Context *_context;
    CVC4::context::CDO<bool> *_cdConstraintActive;

    /* ReluConstraint and AbsoluteValueConstraint use PhaseStatus enumeration.
       MaxConstraint and Disjunction interpret the PhaseStatus value as the case
       number (counts from 1, value 0 is reserved and used as PHASE_NOT_FIXED).
    */
    CVC4::context::CDO<PhaseStatus> *_cdPhaseStatus;

    /*
      Store infeasible cases under the current search prefix. Backtracks with context.
    */
    CVC4::context::CDList<PhaseStatus> *_cdInfeasibleCases;

    /*
      The score denotes priority for splitting. When score is negative, the PL constraint
      is not being considered for splitting.
      We pick the PL constraint with the highest score to branch.
     */
    double _score;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      The gurobi object for solving the LPs during the search.
    */
    GurobiWrapper *_gurobi;

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
    void initializeDuplicateCDOs( PiecewiseLinearConstraint *clone ) const;

    /*
       Check whether a case is marked as infeasible under current search prefix.
     */
    bool isCaseInfeasible( PhaseStatus phase ) const;

    /**********************************************************************/
    /*                         BOUND WRAPPER METHODS                      */
    /**********************************************************************/
    /* These methods prefer using BoundManager over local bound arrays.   */

    /*
       Checks whether lower bound value exists.

       If BoundManager is in use, returns true since it initializes bounds for all variables.
    */
    inline bool existsLowerBound( unsigned var ) const
    {
        return _boundManager != nullptr || _lowerBounds.exists( var );
    }

    /*
       Checks whether upper bound value exists.

       If BoundManager is in use, returns true since it initializes bounds for all variables.
    */
    inline bool existsUpperBound( unsigned var ) const
    {
        return _boundManager != nullptr || _upperBounds.exists( var );
    }

    /*
       Method obtains lower bound of *var*.
     */
    inline double getLowerBound( unsigned var ) const
    {
        return ( _boundManager != nullptr ) ? _boundManager->getLowerBound( var )
                                            : _lowerBounds[var];
    }

    /*
       Method obtains upper bound of *var*.
     */
    inline double getUpperBound( unsigned var ) const
    {
        return ( _boundManager != nullptr ) ? _boundManager->getUpperBound( var )
                                            : _upperBounds[var];
    }

    /*
       Method sets the lower bound of *var* to *value*.
     */
    inline void setLowerBound( unsigned var, double value )
    {
        ( _boundManager != nullptr ) ? _boundManager->setLowerBound( var, value )
                                     : _lowerBounds[var] = value;
    }

    /*
       Method sets the upper bound of *var* to *value*.
     */
    inline void setUpperBound( unsigned var, double value )
    {
        ( _boundManager != nullptr ) ? _boundManager->setUpperBound( var, value )
                                     : _upperBounds[var] = value;
    }

    /**********************************************************************/
    /*                      ASSIGNMENT WRAPPER METHODS                    */
    /**********************************************************************/
    inline bool existsAssignment( unsigned variable ) const
    {
        if ( _gurobi )
            return _gurobi->existsAssignment( Stringf( "x%u", variable ) );
        else if ( _tableau )
            return _tableau->existsValue( variable );
        else
            return false;
    }

    inline double getAssignment( unsigned variable ) const
    {
        if ( _gurobi == nullptr )
        {
            return _tableau->getValue( variable );
        }
        else
            return _gurobi->getAssignment( Stringf( "x%u", variable ) );
    }

    List<unsigned> _tableauAuxVars;
};

#endif // __PiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
