/*********************                                                        */
/*! \file SmtCore.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __SmtCore_h__
#define __SmtCore_h__

#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "Stack.h"

class IEngine;

class SmtCore
{
public:
    enum {
        SPLIT_THRESHOLD = 5,
    };

    SmtCore( IEngine *engine );

    /*
      Inform the SMT core that a PL constraint is violated.
    */
    void reportViolatedConstraint( PiecewiseLinearConstraint *constraint );

    /*
      Returns true iff the SMT core wants to perform a case split.
    */
    bool needToSplit() const;

    /*
      Perform the split according to the constraint marked for
      splitting. Update bounds, add equations and update the stack.
    */
    void performSplit();

private:
    /*
      A stack entry is just a list of case splits to perform.
    */
    typedef List<PiecewiseLinearCaseSplit> Splits;

    /*
      The case-split stack.
    */
    Stack<Splits> _stack;

    /*
      The engine.
    */
    IEngine *_engine;

    /*
      Do we need to perform a split and on which constraint.
    */
    bool _needToSplit;
    PiecewiseLinearConstraint *_constraintForSplitting;

    /*
      Count how many times each constraint has been violated.
    */
    Map<PiecewiseLinearConstraint *, unsigned> _constraintToViolationCount;
};

#endif // __SmtCore_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
