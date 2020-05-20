//
// Created by guyam on 5/20/20.
//

#ifndef MARABOU_SIGNCONSTRAINT_H
#define MARABOU_SIGNCONSTRAINT_H





#include "Map.h"
#include "PiecewiseLinearConstraint.h"


class SignConstraint : public PiecewiseLinearConstraint
{
public:
    enum PhaseStatus {
        PHASE_NOT_FIXED = 0,
        PHASE_POSITIVE = 1,
        PHASE_NEGATIVE = 2,
    };

    SignConstraint( unsigned b, unsigned f );

    //SignConstraint( const String &serializedSign );


    /*
    Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const; // todo check implementation


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
    void notifyVariableValue( unsigned variable, double value ); //TODO implement!!
    void notifyLowerBound( unsigned variable, double bound ); //TODO implement!!
    void notifyUpperBound( unsigned variable, double bound ); //TODO implement!!


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
    List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const;  //TODO implement!!


    /*
      Return a list of smart fixes for violated constraint.
    */
    List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau *tableau ) const;  //TODO implement!!


    /*
      Check if the constraint's phase has been fixed.
    */
    bool phaseFixed() const;



    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    PiecewiseLinearCaseSplit getValidCaseSplit() const;


    /*
     Returns true iff and the constraint has become obsolote as a result of variable eliminations.
    */
    bool constraintObsolete() const;



    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const;  //TODO implement!!



    /*
      Returns string with shape: sign, _f, _b
    */
    String serializeToString() const;


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
    bool haveOutOfBoundVariables() const; // same as ReLU code







};




#endif //MARABOU_SIGNCONSTRAINT_H
