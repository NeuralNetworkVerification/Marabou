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
#include "PiecewiseLinearCaseSplit.h"
#include "Map.h"
#include "Queue.h"

class ITableau;
class String;

class PiecewiseLinearConstraintState
{
public:
    PiecewiseLinearConstraintState() {}
    virtual ~PiecewiseLinearConstraintState() {}

    bool _constraintActive;
    Map<unsigned, double> _assignment;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    Queue<Tightening> _entailedTightenings;
};

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

    PiecewiseLinearConstraint( unsigned f )
        : _f( f )
        , _constraintActive( true )
    {
    }
    virtual ~PiecewiseLinearConstraint() {}

    /*
      Return a clone of the constraint.
    */
    virtual PiecewiseLinearConstraint *duplicateConstraint() const = 0;

    /*
      Register/unregister the constraint with a talbeau.
    */
    virtual void registerAsWatcher( ITableau *tableau ) = 0;
    virtual void unregisterAsWatcher( ITableau *tableau ) = 0;

    /*
      The variable watcher notifcation callbacks.
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
    virtual List<unsigned> getParticiatingVariables() const = 0;

  	/*
	    Return the target variable
  	*/
    virtual unsigned getF() const
    {
      return _f;
    }

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
      Allocate a new state (derived from PiecewiseLinearConstraintState)
      to save into.
    */
    virtual PiecewiseLinearConstraintState *allocateState() const = 0;

    /*
      Store and restore the constraint's state. Needed for case splitting
      and backtracking.
    */
    virtual void storeState( PiecewiseLinearConstraintState &state ) const
    {
        state._constraintActive = _constraintActive;
        state._assignment = _assignment;
        state._lowerBounds = _lowerBounds;
        state._upperBounds = _upperBounds;
        state._entailedTightenings = _entailedTightenings;
    }
    virtual void restoreState( const PiecewiseLinearConstraintState &state )
    {
        _constraintActive = state._constraintActive;
        _assignment = state._assignment;
        _lowerBounds = state._lowerBounds;
        _upperBounds = state._upperBounds;
        _entailedTightenings = state._entailedTightenings;
    }

    /*
      Dump the current state of the constraint.
    */
    virtual void dump( String & ) const {}

	  virtual void changeVarAssign( unsigned prevVar, unsigned newVar) = 0;

  	virtual void eliminateVar( unsigned var, double val) = 0;

  	virtual void deriveTighterBounds( Map<unsigned, double> &, Map<unsigned, double> & ){};

    /*
      Get the tightenings entailed by the constraint.
    */
    Queue<Tightening> &getEntailedTightenings()
    {
        return _entailedTightenings;
    }

protected:
    unsigned _f;
    bool _constraintActive;
    Map<unsigned, double> _assignment;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    Queue<Tightening> _entailedTightenings;
};

#endif // __PiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
