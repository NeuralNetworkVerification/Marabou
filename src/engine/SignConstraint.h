/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __SignConstraint_h__
#define __SignConstraint_h__

#include <cmath>
#include "Map.h"
#include "PiecewiseLinearConstraint.h"

/*
  The Sign function returns +1 for any input x >=0 (including 0),
  and -1 if x < 0
*/

class SignConstraint : public PiecewiseLinearConstraint
{
public:
    enum PhaseStatus {
        PHASE_NOT_FIXED = 0,
        PHASE_POSITIVE = 1,
        PHASE_NEGATIVE = 2,
    };

    /*
      The f variable is the sign output on the b variable:
      f = sign( b )
    */
    SignConstraint( unsigned b, unsigned f );
    SignConstraint( const String &serializedSign );

    /*
      Get the type of this constraint.
    */
    PiecewiseLinearFunctionType getType() const;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
     */
    List<PiecewiseLinearCaseSplit> getCaseSplits() const;

    /*
      Return a clone of the constraint.
    */
    PiecewiseLinearConstraint *duplicateConstraint() const;

    /*
      Restore the state of this constraint from the given one.
    */
    void restoreState( const PiecewiseLinearConstraint *state );

    /*
      Register/unregister the constraint with a talbeau.
     */
    void registerAsWatcher( ITableau *tableau );
    void unregisterAsWatcher( ITableau *tableau );

    /*
      These callbacks are invoked when a watched variable's value
      changes, or when its bounds change.
    */
    void notifyVariableValue( unsigned variable, double value );
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint
    */
    bool participatingVariable( unsigned variable ) const;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const;

    /*
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const;

    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getValidCaseSplit() const;

    /*
      Preprocessing related functions, to inform that a variable has
      been eliminated completely because it was fixed to some value,
      or that a variable's index has changed (e.g., x4 is now called
      x2). constraintObsolete() returns true iff and the constraint
      has become obsolote as a result of variable eliminations.
    */
    void eliminateVariable( unsigned variable, double fixedValue );
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );

    /*
      Returns true iff and the constraint has become obsolote as a
      result of variable eliminations.
    */
    bool constraintObsolete() const;

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const;

    /*
      Dump the current state of the constraint.
    */
    void dump( String &output ) const;

    /*
      Returns string with shape: sign, _f, _b
    */
    String serializeToString() const;

    /*
      Get the index of the B and F variables.
    */
    unsigned getB() const;
    unsigned getF() const;

  bool supportPolarity() const;

  /*
    Return the polarity of the Sign Constraint, which computes how symmetric
    the bound of the input to this ReLU is with respect to 0.
    Let LB be the lowerbound, and UB be the upperbound.
    If LB >= 0, polarity is 1.
    If UB < 0, polarity is -1.
    If LB < 0, and UB > 0, polarity is ( LB + UB ) / (UB - LB).

    We divide the sum by the width of the interval so that the polarity is
    always between -1 and 1. The closer it is to 0, the more symmetric the
    bound is.
  */
  double computePolarity() const;

  /*
    Update the preferred direction for fixing and handling case split
  */
  void updateDirection();

  PhaseStatus getDirection() const;

  void updateScoreBasedOnPolarity();

private:
    unsigned _b, _f;
    PhaseStatus _phaseStatus;

    /*
      Denotes which case split to handle first.
      And which phase status to repair a relu into.
    */
    PhaseStatus _direction;

    PiecewiseLinearCaseSplit getNegativeSplit() const;
    PiecewiseLinearCaseSplit getPositiveSplit() const;

    bool _haveEliminatedVariables;

    /*
      Set the phase status.
    */
    void setPhaseStatus( PhaseStatus phaseStatus );
    static String phaseToString( PhaseStatus phase );

    /*
      Return true iff b or f are out of bounds.
    */
    bool haveOutOfBoundVariables() const;
};

#endif // __SignConstraint_h__
