/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ReluConstraint_h__
#define __ReluConstraint_h__

#include "Map.h"
#include "PiecewiseLinearConstraint.h"

class ReluConstraint : public PiecewiseLinearConstraint
{
public:
    enum PhaseStatus {
        PHASE_NOT_FIXED = 0,
        PHASE_ACTIVE = 1,
        PHASE_INACTIVE = 2,
    };

    ReluConstraint( unsigned b, unsigned f );

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
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
     */
    List<PiecewiseLinearCaseSplit> getCaseSplits() const;

    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getValidCaseSplit() const;

    /*
      Dump the current state of the constraint.
    */
    void dump( String &output ) const;

	void tightenPL( Tightening tighten );

	void preprocessBounds( unsigned variable, double value, Tightening::BoundType type );

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2).
    */
    void eliminateVariable( unsigned variable, double fixedValue );
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );

private:
    unsigned _b, _f;
    PhaseStatus _phaseStatus;

    PiecewiseLinearCaseSplit getInactiveSplit() const;
    PiecewiseLinearCaseSplit getActiveSplit() const;
};

#endif // __ReluConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
