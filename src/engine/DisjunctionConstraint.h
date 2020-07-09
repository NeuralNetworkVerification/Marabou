/*********************                                                        */
/*! \file DisjunctionConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Christopher Lazarus
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __DisjunctionConstraint_h__
#define __DisjunctionConstraint_h__

#include "Map.h"
#include "PiecewiseLinearConstraint.h"

class DisjunctionConstraint : public PiecewiseLinearConstraint
{
public:
    ~DisjunctionConstraint() {};
    DisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts );
    DisjunctionConstraint( const String &serializedDisjunction );

    /*
      Get the type of this constraint.
    */
    PiecewiseLinearFunctionType getType() const;

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
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const;

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
      Dump the current state of the constraint.
    */
    void dump( String &output ) const;

    /*
      For preprocessing: get any auxiliary equations that this
      constraint would like to add to the equation pool. In the Disjunction
      case, this is an equation of the form aux = f - b, where aux is
      non-negative.
    */
    void addAuxiliaryEquations( InputQuery &inputQuery );

    /*
      Ask the piecewise linear constraint to contribute a component to the cost
      function. If implemented, this component should be empty when the constraint is
      satisfied or inactive, and should be non-empty otherwise. Minimizing the returned
      equation should then lead to the constraint being "closer to satisfied".
    */
    virtual void getCostFunctionComponent( Map<unsigned, double> &cost ) const;

    /*
      Returns string with shape: disjunction, _f, _b
    */
    String serializeToString() const;

    /*
      Return true if and only if this piecewise linear constraint supports
      symbolic bound tightening.
    */
    bool supportsSymbolicBoundTightening() const;

private:
    /*
      The disjuncts that form this PL constraint
    */
    List<PiecewiseLinearCaseSplit> _disjuncts;

    /*
      The disjuncts that are still possible, given the current
      lower and upper bounds
    */
    List<PiecewiseLinearCaseSplit> _feasibleDisjuncts;

    /*
      The list of variables that appear in any of the disjuncts
    */
    Set<unsigned> _participatingVariables;

    /*
      Go over the participating disjuncts and extract from them the list
      of participating variables
    */
    void extractParticipatingVariables();

    /*
      Check whether a particular disjunct is satisfied by the current
      assignment
    */
    bool disjunctSatisfied( const PiecewiseLinearCaseSplit &disjunct ) const;

    /*
      Go over the list of disjuncts and find just the ones that are
      still possible, given the current varibale bounds
    */
    void updateFeasibleDisjuncts();
    bool disjunctIsFeasible( const PiecewiseLinearCaseSplit &disjunct ) const;
};

#endif // __DisjunctionConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
