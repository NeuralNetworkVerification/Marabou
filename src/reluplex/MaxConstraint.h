/*********************                                                        */
/*! \file MaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __MaxConstraint_h__
#define __MaxConstraint_h__

#include "Map.h"
#include "PiecewiseLinearConstraint.h"

class MaxConstraint : public PiecewiseLinearConstraint
{
public:
	MaxConstraint( unsigned f, const List<unsigned> &elements );
	~MaxConstraint();

    /*
      Register/unregister the constraint with a talbeau.
    */
	void registerAsWatcher( ITableau *tableau );
    void unregisterAsWatcher( ITableau *tableau );

    /*
      Turn the constraint on/off.
    */
    void setActiveConstraint( bool active );
    bool isActive() const;

    /*
      This callback is invoked when a watched variable's value
      changes.
    */
    void notifyVariableValue( unsigned variable, double value );

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint
    */
    bool participatingVariable( unsigned variable ) const;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticiatingVariables() const;

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
      Store and restore the constraint's state. Needed for case splitting
      and backtracking.
    */
    void storeState( PiecewiseLinearConstraintState &state ) const;
    void restoreState( const PiecewiseLinearConstraintState &state );

    /*
      Allocate a MaxConstraintState for storing state. (TODO)
    */
    PiecewiseLinearConstraintState *allocateState() const;

private:
    bool _constraintActive;
    unsigned _f;
  	unsigned _maxIndex;
    List<unsigned> _elements;

  	Map<unsigned, double> _assignment;
};

#endif // __MaxConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
