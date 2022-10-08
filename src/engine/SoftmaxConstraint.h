/*********************                                                        */
/*! \file SoftmaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
**/

#ifndef __SoftmaxConstraint_h__
#define __SoftmaxConstraint_h__

#include "List.h"
#include "Map.h"
#include "TranscendentalConstraint.h"

#include <cmath>

class SoftmaxConstraint : public TranscendentalConstraint
{
public:
    SoftmaxConstraint( const Vector<unsigned> &inputs,
                       const Vector<unsigned> &outputs );
    SoftmaxConstraint( const String &serializedSoftmax );

    /*
      Get the type of this constraint.
    */
    TranscendentalFunctionType getType() const override;

    /*
      Return a clone of the constraint.
    */
    TranscendentalConstraint *duplicateConstraint() const override;

    /*
      Restore the state of this constraint from the given one.
    */
    void restoreState( const TranscendentalConstraint *state ) override;

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
      Returns true iff the variable participates in this transcendental constraint
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

    String serializeToString() const override;

    const Vector<unsigned> &getInputs() const;
    const Vector<unsigned> &getOutputs() const;

  static void softmax( const Vector<double> &input, Vector<double> &output );

  static void xTilda( const Vector<double> &input, double value,
                      Vector<double> &output );

  static double SE( const Vector<double> &input );

  static double LSE( const Vector<double> &input );

private:
    Vector<unsigned> _inputs;
    Vector<unsigned> _outputs;
};

#endif // __SoftmaxConstraint_h__
