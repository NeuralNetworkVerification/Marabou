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
 ** [[ Add lengthier description here ]]

**/

#ifndef __MaxConstraint_h__
#define __MaxConstraint_h__

#include "Map.h"
#include "PiecewiseLinearConstraint.h"

class MaxConstraint : public PiecewiseLinearConstraint
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

    /*
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const;

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2). constraintObsolete() returns true iff and the constraint has become obsolote
      as a result of variable eliminations.
    */
    void eliminateVariable( unsigned variable, double fixedValue );
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );
    bool constraintObsolete() const;

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const;

    /*
      For preprocessing: get any auxiliary equations that this constraint would
      like to add to the equation pool.
    */
    void addAuxiliaryEquations( InputQuery &inputQuery );

    /*
      Returns string with shape:
      max, _f, element_1, element_2, ... , element_n
    */
    String serializeToString() const;

 private:
    unsigned _f;
    Set<unsigned> _elements;

    unsigned _maxIndex;
    bool _maxIndexSet;
    double _maxLowerBound;
    bool _obsolete;

    void resetMaxIndex();

    /*
      Returns the phase where variable argMax has maximum value.
    */
    PiecewiseLinearCaseSplit getSplit( unsigned argMax ) const;
};

#endif // __MaxConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
