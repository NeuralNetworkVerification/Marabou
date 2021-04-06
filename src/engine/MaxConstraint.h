/*********************                                                        */
/*! \file MaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** MaxConstraint implements the following constraint:
 ** _f = Max( e1, e2, ..., eM), where _elements = { e1, e2, ..., eM}
 **
 ** The constraint will refer to elements as its phases, by wrapping the
 ** variable identifiers as PhaseStatus enumeration. Additionally, during
 ** preprocessing one or more phases may be eliminated from the constraint. A
 ** maximum of such constraints is stored locally, and to denote this phase a
 ** special value PhaseStatus::MAX_PHASE_ELIMINATED is used.
 **
 ** The constraint operates in two modes: pre-processing mode, which stores
 ** bounds locally, and context dependent mode, which is used during the search.
 ** Invoking initializeCDOs method activates the context dependent mode, and the
 ** constraint object synchronizes its state automatically with the central context
 ** object.
 **/

#ifndef __MaxConstraint_h__
#define __MaxConstraint_h__

#include "Map.h"
#include "ContextDependentPiecewiseLinearConstraint.h"

class MaxConstraint : public ContextDependentPiecewiseLinearConstraint
{
public:
    ~MaxConstraint();

    /*
      The f variable is the output of max over the variables stored in elements:
      f = max( elements )
    */
    MaxConstraint( unsigned f, const Set<unsigned> &elements );
    MaxConstraint( const String &serializedMax );

    /*
      Get the type of this constraint.
    */
    PiecewiseLinearFunctionType getType() const override;

    /*
      Return a clone of the constraint.
    */
    ContextDependentPiecewiseLinearConstraint *duplicateConstraint() const override;

    /*
      Restore the state of this constraint from the given one.
    */
    void restoreState( const PiecewiseLinearConstraint *state ) override;

    /*
      Register/unregister the constraint with a talbeau.
    */
    void registerAsWatcher( ITableau *tableau ) override;
    void unregisterAsWatcher( ITableau *tableau ) override;

    /*
      This callback is invoked when a watched variable's value
      changes.
    */
    void notifyVariableValue( unsigned variable, double value ) override;
    void notifyLowerBound( unsigned variable, double value ) override;
    void notifyUpperBound( unsigned variable, double value ) override;

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint
    */
    bool participatingVariable( unsigned variable ) const override;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const override;
    List<unsigned> getElements() const;
    unsigned getF() const;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const override;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const override;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
    */
    List<PiecewiseLinearCaseSplit> getCaseSplits() const override;


    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getImpliedCaseSplit() const override;
    PiecewiseLinearCaseSplit getValidCaseSplit() const override;

    /*
       Returns case split corresponding to the given phase/id
     */
    PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus phase ) const override;

    /*
       Returns a list of all cases - { ABS_POSITIVE, ABS_NEGATIVE }
       The order of returned cases affects the search, and this method is where related
       heuristics should be implemented.
     */
    List<PhaseStatus> getAllCases() const override;

    /*
       Checks whether a case has been flagged as infeasible. Works only in
       context-dependent mode.
     */
    bool isCaseInfeasible( unsigned phase ) const;

    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const override;

    /*
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const override;

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2). constraintObsolete() returns true iff and the constraint has become obsolote
      as a result of variable eliminations.
    */
    void eliminateVariable( unsigned variable, double fixedValue ) override;
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex ) override;
    bool constraintObsolete() const override;

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const override;

    /*
      For preprocessing: get any auxiliary equations that this constraint would
      like to add to the equation pool.
    */
    void addAuxiliaryEquations( InputQuery &inputQuery ) override;

    /*
      Returns string with shape:
      max, _f, element_1, element_2, ... , element_n
    */
    String serializeToString() const override;

    /*
     * Returns a boolean value indicating if at least one input variable was eliminated (True)
     * or not (False)
     */
    bool wereVariablesEliminated() const;



    bool isImplication() const override;

private:
    unsigned _f;
    Set<unsigned> _elements;
    Set<unsigned> _initialElements;

    double _maxLowerBound;
    bool _obsolete;
    bool _eliminatedVariables;
    double _maxValueOfEliminated;

    /*
       Functions that abstract away _maxIndex and _maxIndexSet, and use the
       refactored PhaseStatus to store this information.

       Assumes that the total number of variables does not reach the value of
       MAX_PHASE_ELIMINATED.
     */
    inline bool maxIndexSet() const
    {
        return getPhaseStatus() != PHASE_NOT_FIXED;
    }

    inline void setMaxIndex( unsigned variable )
    {
        setPhaseStatus( variableToPhase( variable ) );
    }

    inline unsigned getMaxIndex() const
    {
        ASSERT( maxIndexSet() );
        return phaseToVariable( getPhaseStatus() );
    }

    inline void clearMaxIndex()
    {
        setPhaseStatus( PHASE_NOT_FIXED );
    }

    void resetMaxIndex();

    /*
      Returns the phase where variable argMax has maximum value.
    */
    PiecewiseLinearCaseSplit getSplit( unsigned argMax ) const;

    /*
       Conversion functions between variables and PhaseStatus.
     */

    inline PhaseStatus variableToPhase( unsigned variable ) const
    {
        return ( variable == MAX_PHASE_ELIMINATED ) ? MAX_PHASE_ELIMINATED :
               static_cast<PhaseStatus>( variable + 1u  );
    }

    inline unsigned phaseToVariable( PhaseStatus phase ) const
    {
        return ( phase == MAX_PHASE_ELIMINATED ) ? MAX_PHASE_ELIMINATED :
               static_cast<unsigned>( phase ) - 1u;
    }


};

#endif // __MaxConstraint_h__
