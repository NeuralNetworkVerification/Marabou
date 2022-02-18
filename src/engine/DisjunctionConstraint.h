/*********************                                                        */
/*! \file DisjunctionConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Christopher Lazarus, Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** DisjunctionConstraint implements the following constraint:
 ** e1 \/ e2 \/ ... \/ eM, where _elements = { e1, e2, ..., eM }
 **
 ** The constraint introduces identifiers for its PiecewiseLinearCaseSplit
 ** elements.
 **
 ** The constraint is implemented as PiecewiseLinearConstraint
 ** and operates in two modes:
 **   * pre-processing mode, which stores bounds locally, and
 **   * context dependent mode, used during the search.
 **
 ** Invoking initializeCDOs method activates the context dependent mode, and the
 ** DisjunctionConstraint object synchronizes its state automatically with the central
 ** Context object.
 **/

#ifndef __DisjunctionConstraint_h__
#define __DisjunctionConstraint_h__

#include "Vector.h"
#include "PiecewiseLinearConstraint.h"

class DisjunctionConstraint : public PiecewiseLinearConstraint
{
public:
    ~DisjunctionConstraint() {};
    DisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts );
    DisjunctionConstraint( const Vector<PiecewiseLinearCaseSplit> &disjuncts );
    DisjunctionConstraint( const String &serializedDisjunction );

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
       Returns a list of all cases. The order of returned cases affects the
       search, and this method is where related heuristics should be
       implemented.
     */
    List<PhaseStatus> getAllCases() const override;

    /*
       Returns case split corresponding to the given phase id.
     */
    PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus phase ) const override;

    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const override;

    /*
      If the constraint's phase has been fixed, get the implied case split.
    */
    PiecewiseLinearCaseSplit getImpliedCaseSplit() const override;

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
      Whether the constraint can contribute the SoI cost function.
    */
    virtual inline bool supportSoI() const override
    {
        return false;
    }

    /*
      Transform the disjunction into a disjunction where each disjunct only
      contains variable bounds.
    */
    void transformToUseAuxVariables( InputQuery &inputQuery ) override;

    /*
      Dump the current state of the constraint.
    */
    void dump( String &output ) const override;

    /*
      Returns string with shape: disjunction, _f, _b
    */
    String serializeToString() const override;

private:
    /*
      The disjuncts that form this PL constraint
    */
    Vector<PiecewiseLinearCaseSplit> _disjuncts;

    /*
      The disjuncts that are still possible, given the current
      lower and upper bounds
    */
    List<unsigned> _feasibleDisjuncts;

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
      still feasible, given the current variable bounds
    */
    void updateFeasibleDisjuncts();
    bool disjunctIsFeasible( unsigned ind ) const;
    bool caseSplitIsFeasible( const PiecewiseLinearCaseSplit & caseSplit ) const;

    inline PhaseStatus indToPhaseStatus( unsigned ind ) const
    {
        return static_cast<PhaseStatus>( ind + 1 );
    }

    inline unsigned phaseStatusToInd( PhaseStatus phase ) const
    {
        //ASSERT( phase != PHASE_NOT_FIXED );
        return static_cast<unsigned>( phase ) - 1;
    }
};

#endif // __DisjunctionConstraint_h__
