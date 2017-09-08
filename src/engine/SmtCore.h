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
#include "Statistics.h"

class EngineState;
class IEngine;
class String;

class SmtCore
{
public:
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

    /*
      Pop an old split from the stack, and perform a new split as
      needed. Return true if successful, false if the stack is empty.
    */
    bool popSplit();

    /*
      The current stack depth.
    */
    unsigned getStackDepth() const;

    /*
      Have the SMT core start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

private:
    /*
      A stack entry is the tableau state before the split, plus a list
      of the case splits to perform.
    */
    struct StackEntry
    {
    public:
        List<PiecewiseLinearCaseSplit> _splits;
        EngineState *_engineState;
    };

    /*
      Collect and print various statistics.
    */
    Statistics *_statistics;

    /*
      The case-split stack.
    */
    Stack<StackEntry> _stack;

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

    static void log( const String &message );
};

#endif // __SmtCore_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
