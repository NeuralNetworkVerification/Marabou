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

#include "Map.h"

class PiecewiseLinearConstraint
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

    virtual ~PiecewiseLinearConstraint() {}

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
      Returns true iff the given assignment satisfies the constraint.
    */
    virtual bool satisfied( const Map<unsigned, double> &assignment ) const = 0;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    virtual List<PiecewiseLinearConstraint::Fix> getPossibleFixes( const Map<unsigned,
                                                                   double> &assignment ) const = 0;
};

#endif // __PiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
