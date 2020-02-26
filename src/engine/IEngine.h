/*********************                                                        */
/*! \file IEngine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __IEngine_h__
#define __IEngine_h__

#include "List.h"

#ifdef _WIN32
#undef ERROR
#endif

class EngineState;
class Equation;
class PiecewiseLinearCaseSplit;
class PiecewiseLinearConstraint;
class IEngine
{
public:
    virtual ~IEngine() {};

    enum ExitCode {
        UNSAT = 0,
        SAT = 1,
        ERROR = 2,
        TIMEOUT = 3,
        QUIT_REQUESTED = 4,

        NOT_DONE = 999,
    };

    /*
      Add equations and apply tightenings from a PL case split.
    */
    virtual void applySplit( const PiecewiseLinearCaseSplit &split ) = 0;

    /*
      Methods for storing and restoring the state of the engine.
    */
    virtual void storeState( EngineState &state, bool storeAlsoTableauState ) const = 0;
    virtual void restoreState( const EngineState &state ) = 0;
    virtual void setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints ) = 0;

    /*
      Solve the encoded query.
    */
    virtual bool solve( unsigned timeoutInSeconds ) = 0;

    /*
      Retrieve the exit code.
    */
    virtual ExitCode getExitCode() const = 0;

    /*
      Methods for DnC: reset the engine state for re-use,
      get input variables.
    */
    virtual void reset() = 0;
    virtual List<unsigned> getInputVariables() const = 0;

    virtual void updateScores()=0;

    /*
      Pick the piecewise linear constraint for branching
    */
    virtual PiecewiseLinearConstraint *pickBranchPLConstraint()=0;

};

#endif // __IEngine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
