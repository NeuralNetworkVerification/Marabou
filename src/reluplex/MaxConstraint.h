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
  	MaxConstraint( unsigned f, const Set<unsigned> &elements );
  	~MaxConstraint();

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
      This callback is invoked when a watched variable's value
      changes.
    */
    void notifyVariableValue( unsigned variable, double value );
  	void notifyLowerBound( unsigned variable, double value );
  	void notifyUpperBound( unsigned variable, double value );

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

	void tightenPL( Tightening tighten, List<Tightening> & tightenings );

	void updateBounds();

	void preprocessBounds( unsigned variable, double value, Tightening::BoundType type );

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2).
    */
    void eliminateVariable( unsigned variable, double fixedValue );
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings );

private:
    unsigned _f;
  	unsigned _maxIndex;
    Set<unsigned> _elements;
    Set<unsigned> _eliminated;
	unsigned _maxElim;
    double _minLowerBound;
    double _maxUpperBound;
    bool _phaseFixed;
    unsigned _fixedPhase;

    /*
      Returns the phase where variable argMax has maximum value.
    */
    PiecewiseLinearCaseSplit getSplit( unsigned argMax ) const;

    /*
      Returns the minimum of the lower bounds of all elements.
    */
    double getMinLowerBound() const;

    /*
      Returns the maximum of the upper bounds of all elements.
    */
    double getMaxUpperBound() const;

	void setLowerBound( unsigned variable, double value );

	void setUpperBound( unsigned variable, double value );


};

#endif // __MaxConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
