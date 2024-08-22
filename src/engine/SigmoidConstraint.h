/*********************                                                        */
/*! \file SigmoidConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori, Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SigmoidConstraint_h__
#define __SigmoidConstraint_h__

#include "List.h"
#include "Map.h"
#include "NonlinearConstraint.h"

#include <cmath>

class SigmoidConstraint : public NonlinearConstraint
{
public:
    SigmoidConstraint( unsigned b, unsigned f );
    SigmoidConstraint( const String &serializedSigmoid );

    /*
      Get the type of this constraint.
    */
    NonlinearFunctionType getType() const override;

    /*
      Return a clone of the constraint.
    */
    NonlinearConstraint *duplicateConstraint() const override;

    /*
      Restore the state of this constraint from the given one.
    */
    void restoreState( const NonlinearConstraint *state ) override;

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
      Returns true iff the variable participates in this nonlinear constraint
    */
    bool participatingVariable( unsigned variable ) const override;

    /*
      Get the list of variables participating in this constraint.
    */
    List<unsigned> getParticipatingVariables() const override;

    /*
      Preprocessing related functions, to inform that a variable has
      been eliminated completely because it was fixed to some value,
      or that a variable's index has changed (e.g., x4 is now called
      x2). constraintObsolete() returns true iff and the constraint
      has become obsolote as a result of variable eliminations.
    */
    void eliminateVariable( unsigned variable, double fixedValue ) override;
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex ) override;
    bool constraintObsolete() const override;

    bool supportVariableElimination() const override
    {
        return true;
    };

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const override;

    /*
      Returns true iff the assignment satisfies the constraint.
    */
    bool satisfied() const override;

    /*
      Dump the current state of the constraint.
    */
    void dump( String &output ) const override;

    /*
      Returns string with shape: sigmoid, _f, _b
    */
    String serializeToString() const override;

    /*
      Get the index of the B and F variables.
    */
    unsigned getB() const;
    unsigned getF() const;

    /*
      Compute the sigmoid function.
    */
    static double sigmoid( double x );

    /*
      Compute the inverse of the sigmoid function.
    */
    static double sigmoidInverse( double y );

    /*
      Compute the derivative of the sigmoid function.
    */
    static double sigmoidDerivative( double x );

    virtual bool attemptToRefine( Query &inputQuery ) const override;

private:
    unsigned _b, _f;
    bool _haveEliminatedVariables;

    static constexpr double INFLECTION_POINT = 0;
};

#endif // __SigmoidConstraint_h__
