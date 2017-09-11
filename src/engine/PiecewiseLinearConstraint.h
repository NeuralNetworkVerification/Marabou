/*********************                                                        */
/*! \file PiecewiseLinearConstraint.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __PiecewiseLinearConstraint_h__
#define __PiecewiseLinearConstraint_h__

#include "ITableau.h"
#include "Map.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Queue.h"
#include "Tightening.h"

class ITableau;
class String;

class PiecewiseLinearConstraint : public ITableau::VariableWatcher
{
public:
    /*
      A possible fix for a violated piecewise linear constraint: a
      variable whose value should be changed.
    */
    struct Fix
    {
    public:
        Fix( unsigned variable, double value )
            : _variable( variable )
            , _value( value )
        {
        }

        unsigned _variable;
        double _value;
    };

    PiecewiseLinearConstraint();

    virtual ~PiecewiseLinearConstraint() {}

    /*
      Return a clone of the constraint.
    */
    virtual PiecewiseLinearConstraint *duplicateConstraint() const = 0;

    /*
      Restore the state of this constraint from the given one.
      We have this function in order to take advantage of the polymorphically
      correct assignment operator.
    */
    virtual void restoreState( const PiecewiseLinearConstraint *state ) = 0;

    /*
      Register/unregister the constraint with a talbeau.
    */
    virtual void registerAsWatcher( ITableau *tableau ) = 0;
    virtual void unregisterAsWatcher( ITableau *tableau ) = 0;

    /*
      The variable watcher notifcation callbacks, about a change in a variable's value or bounds.
    */
    virtual void notifyVariableValue( unsigned /* variable */, double /* value */ ) {}
    virtual void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    virtual void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}

    /*
      Turn the constraint on/off.
    */
    virtual void setActiveConstraint( bool active )
    {
        _constraintActive = active;
    }

    virtual bool isActive() const
    {
        return _constraintActive;
    }

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint.
    */
    virtual bool participatingVariable( unsigned variable ) const = 0;

    /*
      Get the list of variables participating in this constraint.
    */
    virtual List<unsigned> getParticipatingVariables() const = 0;

    /*
      Returns true iff the assignment satisfies the constraint.
    */
    virtual bool satisfied() const = 0;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    virtual List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const = 0;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
    */
    virtual List<PiecewiseLinearCaseSplit> getCaseSplits() const = 0;

    /*
      Check if the constraint's phase has been fixed.
    */
    virtual bool phaseFixed() const = 0;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    virtual PiecewiseLinearCaseSplit getValidCaseSplit() const = 0;

    /*
      Dump the current state of the constraint.
    */
    virtual void dump( String & ) const {}

    /*
      Preprocessing related functions, to inform that a variable has been eliminated completely
      because it was fixed to some value, or that a variable's index has changed (e.g., x4 is now
      called x2).
    */
    virtual void eliminateVariable( unsigned variable, double fixedValue ) = 0;
    virtual void updateVariableIndex( unsigned oldIndex, unsigned newIndex ) = 0;

    /*
      Get the tightenings entailed by the constraint.
    */
    virtual void getEntailedTightenings( List<Tightening> &tightenings ) const = 0;

    /*
      Return true if and only if the constraint has been made meaningless due to
      variable eliminiation and should be eliminated.
    */
    virtual bool constraintDisabledByVariableElimination() const = 0;

    void setStatistics( Statistics *statistics );

	bool _removePL;

protected:
    bool _constraintActive;
	Map<unsigned, double> _assignment;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;

    /*
      Statistics collection
    */
    Statistics *_statistics;
};

#endif // __PiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
