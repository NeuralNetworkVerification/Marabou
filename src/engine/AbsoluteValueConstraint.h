/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Shiran Aziz, Guy Katz, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** AbsoluteValueConstraint implements the following constraint:
 ** f = Abs( b ) =     ( b > 0 -> f =  b )
 **                 /\ ( b <=0 -> f = -b )
 **
 ** It distinguishes two relevant phases for search:
 ** ABS_PHASE_POSITIVE: b > 0
 ** ABS_PHASE_NEGATIVE: b <=0
 **
 ** The constraint is implemented as PiecewiseLinearConstraint
 ** and operates in two modes:
 **   * pre-processing mode, which stores bounds locally, and
 **   * context dependent mode, used during the search.
 **
 ** Invoking initializeCDOs method activates the context dependent mode, and the
 ** AbsoluteValueConstraint object synchronizes its state automatically with the central
 ** Context object.
 **/

#ifndef __AbsoluteValueConstraint_h__
#define __AbsoluteValueConstraint_h__

#include "PiecewiseLinearConstraint.h"

class AbsoluteValueConstraint : public PiecewiseLinearConstraint
{
public:
    /*
      The f variable is the absolute value of the b variable:
      f = | b |
    */
    AbsoluteValueConstraint( unsigned b, unsigned f );
    AbsoluteValueConstraint( const String &serializedAbs );

    /*
      Get the type of this constraint.
    */
    PiecewiseLinearFunctionType getType() const override;

    /*
      Return a clone of the constraint.
    */
    PiecewiseLinearConstraint *duplicateConstraint() const override;

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
      These callbacks are invoked when a watched variable's value
      changes, or when its bounds change.
    */
    void notifyLowerBound( unsigned variable, double bound ) override;
    void notifyUpperBound( unsigned variable, double bound ) override;

    /*
       Returns true iff the variable participates in this piecewise
       linear constraint.
     */
    bool participatingVariable( unsigned variable ) const override;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const override;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const override;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const override;

    /*
      Return a list of smart fixes for violated constraint.
      Currently not implemented, just calls getPossibleFixes().
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const override;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into:

      y = |x| <-->
         ( x <= 0 /\ y = -x ) \/ ( x >= 0 /\ y = x )
    */
    List<PiecewiseLinearCaseSplit> getCaseSplits() const override;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getValidCaseSplit() const override;

    /*
       Returns a list of all cases - { ABS_POSITIVE, ABS_NEGATIVE }
       The order of returned cases affects the search, and this method is where related
       heuristics should be implemented.
     */
    List<PhaseStatus> getAllCases() const override;

    /*
       Returns case split corresponding to the given phase/id
     */
    PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus phase ) const override;

    /*
     * If the constraint's phase has been fixed, get the (valid) case split.
     */
    PiecewiseLinearCaseSplit getImpliedCaseSplit() const override;

    /*
      Check whether the constraint's phase has been fixed.
     */
    void fixPhaseIfNeeded();
    bool phaseFixed() const override;

    /*
      Preprocessing related functions, to inform that a variable has
      been eliminated completely because it was fixed to some value,
      or that a variable's index has changed (e.g., x4 is now called
      x2). constraintObsolete() returns true iff and the constraint
      has become obsolote as a result of variable eliminations.
     */
    void eliminateVariable( unsigned variable, double fixedValue ) override;
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex ) override;
    bool constraintObsolete() const override;

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const override;

    /*
      Dump the current state of the constraint.
    */
    void dump( String &output ) const override;

    /*
      For preprocessing: get any auxiliary equations that this constraint would
      like to add to the equation pool. This way, case splits will be bound
      update of the aux variables.
    */
    void transformToUseAuxVariables( Query &inputQuery ) override;

    /*
      Whether the constraint can contribute the SoI cost function.
    */
    virtual inline bool supportSoI() const override
    {
        return true;
    }

    /*
      Ask the piecewise linear constraint to add its cost term corresponding to
      the given phase to the cost function. The cost term for Abs is:
      _f - _b for the active phase
      _f + _b for the inactive phase
    */
    virtual void getCostFunctionComponent( LinearExpression &cost,
                                           PhaseStatus phase ) const override;

    /*
      Return the phase status corresponding to the values of the *input*
      variables in the given assignment.
    */
    virtual PhaseStatus
    getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const override;

    /*
      Returns string with shape: absoluteValue,_f,_b
     */
    String serializeToString() const override;

    inline unsigned getB() const
    {
        return _b;
    };

    inline unsigned getF() const
    {
        return _f;
    };

    inline bool auxVariablesInUse() const
    {
        return _auxVarsInUse;
    };

    inline unsigned getPosAux() const
    {
        return _posAux;
    };
    inline unsigned getNegAux() const
    {
        return _negAux;
    };

    const List<unsigned> getNativeAuxVars() const override;

private:
    /*
      The variables that make up this constraint; _f = | _b |.
    */
    unsigned _b, _f;

    /*
      Auxiliary variables
    */
    unsigned _posAux, _negAux;
    bool _auxVarsInUse;

    /*
      True iff _b or _f have been eliminated.
    */
    bool _haveEliminatedVariables;

    static String phaseToString( PhaseStatus phase );

    /*
      The two case splits.
    */
    PiecewiseLinearCaseSplit getPositiveSplit() const;
    PiecewiseLinearCaseSplit getNegativeSplit() const;

    /*
      Return true iff _b and _f are not both within bounds.
    */
    bool haveOutOfBoundVariables() const;

    std::shared_ptr<TableauRow> _posTighteningRow;
    std::shared_ptr<TableauRow> _negTighteningRow;

    /*
     Create a tableau row used for explaining bound tightening caused by the constraint's positive
     phase, stored in _posTighteningRow
    */
    void createPosTighteningRow();

    /*
     Create a tableau row used for explaining bound tightening caused by the constraint's negative
     phase, stored in _negTighteningRow
    */
    void createNegTighteningRow();

    /*
      Assign a variable as an aux variable by the tableau, related to some existing aux variable.
    */
    void addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar ) override;
};

#endif // __AbsoluteValueConstraint_h__
