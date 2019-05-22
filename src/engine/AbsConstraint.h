//
// Created by shirana on 5/22/19.
//

#ifndef MARABOU_ABSCONSTRAINT_H
#define MARABOU_ABSCONSTRAINT_H




#include "PiecewiseLinearConstraint.h"


class AbsConstraint : public PiecewiseLinearConstraint {

public:
    enum PhaseStatus {
        PHASE_NOT_FIXED = 0,
        PHASE_POSITIVE = 1,
        PHASE_NEGATIVE = 2,
    };

    AbsConstraint(unsigned b, unsigned f);

    /*
    Return a clone of the constraint.
  */
    PiecewiseLinearConstraint *duplicateConstraint() const;

    /*
      Restore the state of this constraint from the given one.
    */
    void restoreState(const PiecewiseLinearConstraint *state);

    /*
      Register/unregister the constraint with a talbeau.
     */
    void registerAsWatcher(ITableau *tableau);

    void unregisterAsWatcher(ITableau *tableau);

//    /*
//    These callbacks are invoked when a watched variable's value
//    changes, or when its bounds change.
//  */
//    void notifyVariableValue( unsigned variable, double value );
//    void notifyLowerBound( unsigned variable, double bound );
//    void notifyUpperBound( unsigned variable, double bound );


    /*
       Returns true iff the variable participates in this piecewise
       linear constraint.
     */
    bool participatingVariable(unsigned variable) const;

//    /*
//      Get the list of variables participating in this constraint.
//    */
//    List<unsigned> getParticipatingVariables() const;

    /*
      Returns true iff the assignment satisfies the constraint
    */
    bool satisfied() const;


private:
    unsigned _b, _f;
}

#endif //MARABOU_ABSCONSTRAINT_H
