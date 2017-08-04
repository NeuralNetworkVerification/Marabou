/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ReluConstraint_h__
#define __ReluConstraint_h__

#include "PiecewiseLinearConstraint.h"
#include "Queue.h"

class ReluConstraint : public PiecewiseLinearConstraint
{
public:
	  enum StuckStatus {
		  NOT_STUCK = 0,
		  STUCK_ACTIVE,
      STUCK_INACTIVE
    };

    ReluConstraint( unsigned b, unsigned f );

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
      Store and restore the constraint's state. Needed for case splitting
      and backtracking.
    */
    void storeState( PiecewiseLinearConstraintState &state ) const;
    void restoreState( const PiecewiseLinearConstraintState &state );

private:
    unsigned _b;
    unsigned _f;

    Map<unsigned, double> _assignment;

    PiecewiseLinearCaseSplit getInactiveSplit( unsigned auxVariable ) const;
    PiecewiseLinearCaseSplit getActiveSplit( unsigned auxVariable ) const;

    StuckStatus _stuck;
};

class ReluConstraintStateData : public PiecewiseLinearConstraintStateData
{
public:
    Map<unsigned, double> _assignment;
    ReluConstraint::StuckStatus _stuck;
};

#endif // __ReluConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
