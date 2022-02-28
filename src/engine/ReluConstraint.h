/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Christopher Lazarus
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** ReluConstraint implements the following constraint:
 ** f = Max( 0, b ) =    ( b > 0 -> f > 0 )
 **                   /\ ( b <=0 -> f = 0 )
 **
 ** It distinguishes two relevant phases for search:
 ** RELU_PHASE_ACTIVE   : b > 0 and f > 0
 ** RELU_PHASE_INACTIVE : b <=0 and f = 0
 **
 ** The constraint is implemented as PiecewiseLinearConstraint
 ** and operates in two modes:
 **   * pre-processing mode, which stores bounds locally, and
 **   * context dependent mode, used during the search.
 **
 ** Invoking initializeCDOs method activates the context dependent mode, and the
 ** ReluConstraint object synchronizes its state automatically with the central
 ** Context object.
 **/

#ifndef __ReluConstraint_h__
#define __ReluConstraint_h__

#include "LinearExpression.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"

#include <cmath>

class ReluConstraint : public PiecewiseLinearConstraint
{
public:
    /*
      The f variable is the relu output on the b variable:
      f = relu( b )
    */
    ReluConstraint( unsigned b, unsigned f );
    ReluConstraint( const String &serializedRelu );

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
      linear constraint
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
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const override;

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
    PiecewiseLinearCaseSplit getValidCaseSplit() const override;

    /*
       Returns a list of all cases - { RELU_ACTIVE, RELU_INACTIVE}
       The order of returned cases affects the search, and this method is where related
       heuristics should be implemented.
     */
    List<PhaseStatus> getAllCases() const override;

    /*
       Returns case split corresponding to the given phase/id
     */
    PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus phase ) const override;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getImpliedCaseSplit() const override;

    /*
      Check if the constraint's phase has been fixed.
    */
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
      For preprocessing: get any auxiliary equations that this
      constraint would like to add to the equation pool. In the ReLU
      case, this is an equation of the form aux = f - b, where aux is
      non-negative. This way, case splits will be bound
      update of the aux variables.
    */
    void transformToUseAuxVariables( InputQuery &inputQuery ) override;

    /*
      Whether the constraint can contribute the SoI cost function.
    */
    virtual inline bool supportSoI() const override
    {
        return true;
    }

    /*
      Ask the piecewise linear constraint to add its cost term corresponding to
      the given phase to the cost function. The cost term for ReLU is:
        _f - _b for the active phase
        _f      for the inactive phase
    */
    virtual void getCostFunctionComponent( LinearExpression &cost,
                                           PhaseStatus phase ) const override;

    /*
      Return the phase status corresponding to the values of the *input*
      variables in the given assignment.
    */
    virtual PhaseStatus getPhaseStatusInAssignment( const Map<unsigned, double>
                                                    &assignment ) const override;

    /*
      Returns string with shape: relu, _f, _b
    */
    String serializeToString() const override;

    /*
      Get the index of the B and F variables.
    */
    unsigned getB() const;
    unsigned getF() const;

    /*
      Check if the aux variable is in use and retrieve it
    */
    bool auxVariableInUse() const;
    unsigned getAux() const;

    bool supportPolarity() const override;

    /*
      Return the polarity of this ReLU, which computes how symmetric
      the bound of the input to this ReLU is with respect to 0.
      Let LB be the lowerbound, and UB be the upperbound.
      If LB >= 0, polarity is 1.
      If UB <= 0, polarity is -1.
      If LB < 0, and UB > 0, polarity is ( LB + UB ) / (UB - LB).

      We divide the sum by the width of the interval so that the polarity is
      always between -1 and 1. The closer it is to 0, the more symmetric the
      bound is.
    */
    double computePolarity() const;

    /*
      Update the preferred direction for fixing and handling case split
    */
    void updateDirection() override;


    PhaseStatus getDirection() const;

    void updateScoreBasedOnPolarity() override;

private:
    unsigned _b, _f;
    bool _auxVarInUse;
    unsigned _aux;

    /*
      Denotes which case split to handle first.
      And which phase status to repair a relu into.
    */
    PhaseStatus _direction;

    PiecewiseLinearCaseSplit getInactiveSplit() const;
    PiecewiseLinearCaseSplit getActiveSplit() const;

    bool _haveEliminatedVariables;

    static String phaseToString( PhaseStatus phase );

    /*
      Return true iff b or f are out of bounds.
    */
    bool haveOutOfBoundVariables() const;
};

#endif // __ReluConstraint_h__
