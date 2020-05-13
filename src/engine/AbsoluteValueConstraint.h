/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Shiran Aziz, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __AbsoluteValueConstraint_h__
#define __AbsoluteValueConstraint_h__

#include "PiecewiseLinearConstraint.h"

class AbsoluteValueConstraint : public PiecewiseLinearConstraint
{

public:
    enum PhaseStatus {
        PHASE_NOT_FIXED = 0,
        PHASE_POSITIVE = 1,
        PHASE_NEGATIVE = 2,
    };

    /*
      The f variable is the absolute value of the b variable:
      f = | b |
    */
    AbsoluteValueConstraint( unsigned b, unsigned f );

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
    void registerAsWatcher( ITableau *tableau);
    void unregisterAsWatcher( ITableau *tableau);

    /*
      These callbacks are invoked when a watched variable's value
      changes, or when its bounds change.
    */
    void notifyVariableValue( unsigned variable, double value );
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
       Returns true iff the variable participates in this piecewise
       linear constraint.
     */
    bool participatingVariable( unsigned variable ) const;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const;

    /*
      Return a list of smart fixes for violated constraint.
      Currently not implemented, just calls getPossibleFixes().
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into:

      y = |x| <-->
         ( x <= 0 /\ y = -x ) \/ ( x >= 0 /\ y = x )
    */
    List<PiecewiseLinearCaseSplit> getCaseSplits() const;

    /*
      Check whether the constraint's phase has been fixed.
    */
    void fixPhaseIfNeeded();
    bool phaseFixed() const;

    /*
     * If the constraint's phase has been fixed, get the (valid) case split.
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
    bool constraintObsolete() const;

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const;

    /*
      For preprocessing: get any auxiliary equations that this
      constraint would like to add to the equation pool. In the ReLU
      case, this is an equation of the form aux = f - b, where aux is
      non-negative.
    */
    void getAuxiliaryEquations( List<Equation> &newEquations ) const;

    /*
      Returns string with shape: absoluteValue,_f,_b
     */
    String serializeToString() const;

    /*
      Return true if and only if this piecewise linear constraint supports
      symbolic bound tightening.
    */
    bool supportsSymbolicBoundTightening() const;

private:
    /*
      The variables that make up this constraint; _f = | _b |.
    */
    unsigned _b, _f;

    /*
      True iff _b or _f have been eliminated.
    */
    bool _haveEliminatedVariables;

    /*
      The phase status of this constraint: positive, negative, or not
      yet fixed.
    */
    PhaseStatus _phaseStatus;
    void setPhaseStatus( PhaseStatus phaseStatus );

    /*
      The two case splits.
    */
    PiecewiseLinearCaseSplit getPositiveSplit() const;
    PiecewiseLinearCaseSplit getNegativeSplit() const;
};

#endif // __AbsoluteValueConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
