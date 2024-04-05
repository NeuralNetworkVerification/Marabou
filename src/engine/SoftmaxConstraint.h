/*********************                                                        */
/*! \file SoftmaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** SoftmaxConstraint implements the following constraint:
 ** f_i = e^b_i / (e^b_1 + ... + e^b_n) for each of the Softmax output
 **
 **/

#ifndef __SoftmaxConstraint_h__
#define __SoftmaxConstraint_h__

#include "List.h"
#include "Map.h"
#include "NonlinearConstraint.h"

#include <cmath>

class SoftmaxConstraint : public NonlinearConstraint
{
public:
    SoftmaxConstraint( const Vector<unsigned> &inputs, const Vector<unsigned> &outputs );
    SoftmaxConstraint( const String &serializedSoftmax );

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

    /*
      Get the tightenings entailed by the constraint.
    */
    void getEntailedTightenings( List<Tightening> &tightenings ) const override;

    /*
      Returns true iff the assignment satisfies the constraint.
    */
    bool satisfied() const override;

    String serializeToString() const override;

    const Vector<unsigned> &getInputs() const;
    const Vector<unsigned> &getOutputs() const;
    unsigned getOutput( unsigned input ) const;

    static void softmax( const Vector<double> &input, Vector<double> &output );

    // Given a vector input, subtract value from each element in the vector and
    // store the result in output.
    static void xTilda( const Vector<double> &input, double value, Vector<double> &output );

    static double sumOfExponential( const Vector<double> &input );

    static double logSumOfExponential( const Vector<double> &input );

private:
    Vector<unsigned> _inputs;
    Vector<unsigned> _outputs;
    Map<unsigned, unsigned> _inputToOutput;
};

#endif // __SoftmaxConstraint_h__
