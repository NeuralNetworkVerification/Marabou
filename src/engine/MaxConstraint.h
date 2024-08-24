/*********************                                                        */
/*! \file MaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Derek Huang, Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** MaxConstraint implements the following constraint:
 ** _f = Max( e1, e2, ..., eM), where _elements = { e1, e2, ..., eM}
 **
 ** MaxConstraint refers to elements as its phases, by wrapping the variable
 ** identifiers as PhaseStatus enumeration. It allows elimination of elements
 ** during preprocessing. Eliminated elements are abstracted in an aggregate
 ** local member _maxValueOfEliminated, and its phase is a reserved value
 ** PhaseStatus::MAX_PHASE_ELIMINATED.
 **
 ** The constraint is implemented as PiecewiseLinearConstraint
 ** and operates in two modes:
 **   * pre-processing mode, which stores bounds locally, and
 **   * context dependent mode, used during the search.
 **
 ** Invoking initializeCDOs method activates the context dependent mode, and the
 ** MaxConstraint object synchronizes its state automatically with the central
 ** Context object.
 **
 ** Invariants to maintain in this class:
 ** 1. All methods called after transformToUseAuxVariablesIfNeeded
 **    are operating under the assumption that f >= element for each
 **    element in _element, and f >= _maxValueofEliminatedVariables
 ** 2. _maxLowerBound keeps track of the maximal lower bound of the output of
 **    the MaxConstraint, it can be updated by notifyLowerBound(),
 **    notifyUpperBound(), eliminateVariable()
 ** 3. _phaseStatus is updated by notifyLowerBound(), notifyUpperBound(),
       eliminateVariable().
 ** 4. elements in _elements are feasible (wrt. current variable bounds) and
 **    _haveFeasibleEliminatedVariables are up-to-date. They are updated by
 **     notifyLowerBound(), notifyUpperBound(), eliminateVariable().
 ** 5. The constraint is _obsolete only when 1) all elements are eliminated
 **     (handled by eliminateVariable()); 2) _f is eliminated.
 **/

#ifndef __MaxConstraint_h__
#define __MaxConstraint_h__

#include "LinearExpression.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"

#define MAX_VARIABLE_TO_PHASE_OFFSET 1

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
      This callback is invoked when a watched variable's value
      changes.
    */
    void notifyLowerBound( unsigned variable, double value ) override;
    void notifyUpperBound( unsigned variable, double value ) override;

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint
    */
    bool participatingVariable( unsigned variable ) const override;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const override;
    List<unsigned> getElements() const;
    unsigned getF() const;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const override;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const override;

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
    PiecewiseLinearCaseSplit getImpliedCaseSplit() const override;
    PiecewiseLinearCaseSplit getValidCaseSplit() const override;

    /*
       Returns case split corresponding to the given phase/id
     */
    PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus phase ) const override;

    /*
       Returns a list of all cases - { ABS_POSITIVE, ABS_NEGATIVE }
       The order of returned cases affects the search, and this method is where related
       heuristics should be implemented.
     */
    List<PhaseStatus> getAllCases() const override;

    /*
       Checks whether a case has been flagged as infeasible. Works only in
       context-dependent mode.
     */
    bool isCaseInfeasible( unsigned phase ) const;

    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const override;

    /*
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const override;

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
      Represent the Max constraint using the aux variables.
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
      the given phase to the cost function. The cost term for Max is:
      _f - element_i for each element
    */
    virtual void getCostFunctionComponent( LinearExpression &cost,
                                           PhaseStatus phase ) const override;

    virtual PhaseStatus
    getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const override;

    /*
      Returns string with shape:
      max, _f, element_1, element_2, ... , element_n
    */
    String serializeToString() const override;

    bool isImplication() const override;

    inline Set<unsigned> getEliminatedElements() const
    {
        return _eliminatedElements;
    }

    inline Set<unsigned> getParticipatingElements() const
    {
        Set<unsigned> participatingElements = {};
        for ( const auto &element : _elements )
            participatingElements.insert( element );

        for ( const auto &element : _eliminatedElements )
            participatingElements.insert( element );

        return participatingElements;
    }

    inline double getMaxValueOfEliminatedPhases() const
    {
        return _maxValueOfEliminatedPhases;
    }

    inline unsigned getAuxToElement( unsigned element )
    {
        return _auxToElement[element];
    }

    /*
       Conversion functions between variables and PhaseStatus.
    */
    inline PhaseStatus variableToPhase( unsigned variable ) const
    {
        return ( variable == MAX_PHASE_ELIMINATED )
                 ? MAX_PHASE_ELIMINATED
                 : static_cast<PhaseStatus>( variable + MAX_VARIABLE_TO_PHASE_OFFSET );
    }

    inline unsigned phaseToVariable( PhaseStatus phase ) const
    {
        return ( phase == MAX_PHASE_ELIMINATED )
                 ? MAX_PHASE_ELIMINATED
                 : static_cast<unsigned>( phase ) - MAX_VARIABLE_TO_PHASE_OFFSET;
    }

private:
    unsigned _f;
    Set<unsigned> _elements;
    Set<unsigned> _initialElements;

    Map<unsigned, unsigned> _auxToElement;
    Map<unsigned, unsigned> _elementToAux;

    Map<unsigned, unsigned> _elementToTableauAux;
    Map<unsigned, std::shared_ptr<TableauRow>> _elementToTighteningRow;
    Set<unsigned> _eliminatedElements;
    Set<unsigned> _proofEliminatedElements;

    bool _obsolete;

    /*
      We eagerly keep track of the max lower bound of the inputs.
    */
    double _maxLowerBound;

    /*
      We keep track of eliminated variables and the maximal value. The flag is
      true iff some input variables of the max constraint have been eliminated
      (fixed to a constant) and the case split corresponding to one of them is
      feasible (wrt. variable bounds).
    */
    bool _haveFeasibleEliminatedPhases;
    double _maxValueOfEliminatedPhases;

    /*
      Returns the phase where variable argMax has maximum value.
    */
    PiecewiseLinearCaseSplit getSplit( unsigned argMax ) const;


    /*
      Eliminate the case corresponding to the given input variable to Max.
    */
    void eliminateCase( unsigned variable );

    /*
      Return true iff f or the elements are not all within bounds.
    */
    bool haveOutOfBoundVariables() const;

    void createElementTighteningRow( unsigned element );
    const List<unsigned> getNativeAuxVars() const override;
    void addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar ) override;

    /*
      Apply tightenings in the list, discovered by getEntailedTightenings
    */
    void applyTightenings( const List<Tightening> &tightenings ) const;
};

#endif // __MaxConstraint_h__
