/*********************                                                        */
/*! \file PiecewiseLinearCaseSplit.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __PiecewiseLinearCaseSplit_h__
#define __PiecewiseLinearCaseSplit_h__

class PiecewiseLinearCaseSplit
{
public:
    /*
      Store information regarding a bound tightening: which var, which
      bound, and new bound value.
    */
    void setBoundTightening( unsigned variable, bool upperBound, double newBound );

    unsigned getVariable() const;
    bool getUpperBound() const;
    double getNewBound() const;

private:
    /* Bound tightening information. */

    /*
      The variable whos bound is being tightened.
    */
    unsigned _variable;

    /*
      True iff the upper bound is being tightened.
    */
    bool _upperBound;

    /*
      Value of the new bound.
    */
    double _newBound;
};

#endif // __PiecewiseLinearCaseSplit_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
