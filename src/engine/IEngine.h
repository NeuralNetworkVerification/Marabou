/*********************                                                        */
/*! \file IEngine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __IEngine_h__
#define __IEngine_h__

#include "BoundExplainer.h"
#include "DivideStrategy.h"
#include "List.h"
#include "PlcLemma.h"
#include "SnCDivideStrategy.h"
#include "TableauStateStorageLevel.h"
#include "Vector.h"
#include "context/context.h"

#ifdef _WIN32
#undef ERROR
#endif

class EngineState;
class Equation;
class PiecewiseLinearCaseSplit;
class PLCLemma;
class SmtState;
class String;
class PiecewiseLinearConstraint;
class UnsatCertificateNode;

class IEngine
{
public:
    virtual ~IEngine(){};

    enum ExitCode {
        UNSAT = 0,
        SAT = 1,
        ERROR = 2,
        UNKNOWN = 3,
        TIMEOUT = 4,
        QUIT_REQUESTED = 5,

        NOT_DONE = 999,
    };

    /*
      Add equations and apply tightenings from a PL case split.
    */
    virtual void applySplit( const PiecewiseLinearCaseSplit &split ) = 0;

    /*
      Register initial SnC split
    */
    virtual void applySnCSplit( PiecewiseLinearCaseSplit split, String queryId ) = 0;
    virtual bool inSnCMode() const = 0;

    /*
      Hooks invoked before/after context push/pop to store/restore/update context independent data.
    */
    virtual void preContextPushHook() = 0;
    virtual void postContextPopHook() = 0;

    /*
      Methods for storing and restoring the state of the engine.
    */
    virtual void storeState( EngineState &state, TableauStateStorageLevel level ) const = 0;
    virtual void restoreState( const EngineState &state ) = 0;
    virtual void setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints ) = 0;

    /*
      Store the current stack of the smtCore into smtState
    */
    virtual void storeSmtState( SmtState &smtState ) = 0;

    /*
      Apply the stack to the newly created SmtCore, returns false if UNSAT is
      found in this process.
    */
    virtual bool restoreSmtState( SmtState &smtState ) = 0;

    /*
      Solve the encoded query.
    */
    virtual bool solve( double timeoutInSeconds ) = 0;

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

    /*
      Pick the piecewise linear constraint for internal splitting
    */
    virtual PiecewiseLinearConstraint *pickSplitPLConstraint( DivideStrategy strategy ) = 0;

    /*
      Pick the piecewise linear constraint for SnC splitting
    */
    virtual PiecewiseLinearConstraint *pickSplitPLConstraintSnC( SnCDivideStrategy strategy ) = 0;
    /*
      Return the value of a variable bound, as expressed by the bounds explainer and the initial
      bounds
    */
    virtual double explainBound( unsigned var, bool isUpper ) const = 0;

    /*
     * Update the ground bounds
     */
    virtual void updateGroundUpperBound( unsigned var, double value ) = 0;
    virtual void updateGroundLowerBound( unsigned var, double value ) = 0;

    virtual void applyAllBoundTightenings() = 0;

    virtual bool applyAllValidConstraintCaseSplits() = 0;
    /*
      Get Context reference
     */
    virtual CVC4::context::Context &getContext() = 0;

    virtual bool consistentBounds() const = 0;

    /*
      Returns true iff the engine is in proof production mode
    */
    virtual bool shouldProduceProofs() const = 0;

    /*
      Get the ground bound of the variable
    */
    virtual double getGroundBound( unsigned var, bool isUpper ) const = 0;

    /*
      Get the current pointer in the UNSAT certificate node
    */
    virtual UnsatCertificateNode *getUNSATCertificateCurrentPointer() const = 0;

    /*
      Set the current pointer in the UNSAT certificate
    */
    virtual void setUNSATCertificateCurrentPointer( UnsatCertificateNode *node ) = 0;

    /*
      Get the root of the UNSAT certificate proof tree
    */
    virtual const UnsatCertificateNode *getUNSATCertificateRoot() const = 0;

    /*
      Certify the UNSAT certificate
    */
    virtual bool certifyUNSATCertificate() = 0;

    /*
      Finds the variable causing failure and updates its bounds explanations
    */
    virtual void explainSimplexFailure() = 0;

    /*
      Get the boundExplainer
    */
    virtual const BoundExplainer *getBoundExplainer() const = 0;

    /*
      Set the boundExplainer content
    */
    virtual void setBoundExplainerContent( BoundExplainer *boundExplainer ) = 0;

    /*
      Propagate bound tightenings stored in the BoundManager
    */
    virtual void propagateBoundManagerTightenings() = 0;

    /*
      Add lemma to the UNSAT Certificate
    */
    virtual void addPLCLemma( std::shared_ptr<PLCLemma> &explanation ) = 0;
};

#endif // __IEngine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
