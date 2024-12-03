/*********************                                                        */
/*! \file MockEngine.h
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

#ifndef __MockEngine_h__
#define __MockEngine_h__

#include "IEngine.h"
#include "List.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "context/context.h"

class String;

class MockEngine : public IEngine
{
public:
    MockEngine()
    {
        wasCreated = false;
        wasDiscarded = false;

        lastStoredState = NULL;
    }

    ~MockEngine()
    {
    }

    bool wasCreated;
    bool wasDiscarded;

    void mockConstructor()
    {
        TS_ASSERT( !wasCreated );
        wasCreated = true;
    }

    void mockDestructor()
    {
        TS_ASSERT( wasCreated );
        TS_ASSERT( !wasDiscarded );
        wasDiscarded = true;
    }

    struct Bound
    {
        Bound( unsigned variable, double bound )
        {
            _variable = variable;
            _bound = bound;
        }

        unsigned _variable;
        double _bound;
    };

    List<Bound> lastLowerBounds;
    List<Bound> lastUpperBounds;
    List<Equation> lastEquations;
    void applySplit( const PiecewiseLinearCaseSplit &split )
    {
        List<Tightening> bounds = split.getBoundTightenings();
        auto equations = split.getEquations();
        for ( auto &it : equations )
        {
            lastEquations.append( it );
        }

        for ( auto &bound : bounds )
        {
            if ( bound._type == Tightening::LB )
            {
                lastLowerBounds.append( Bound( bound._variable, bound._value ) );
            }
            else
            {
                lastUpperBounds.append( Bound( bound._variable, bound._value ) );
            }
        }
    }

    void postContextPopHook(){};
    void preContextPushHook(){};

    mutable EngineState *lastStoredState;
    void storeState( EngineState &state, TableauStateStorageLevel /*level*/ ) const
    {
        lastStoredState = &state;
    }

    const EngineState *lastRestoredState;
    void restoreState( const EngineState &state )
    {
        lastRestoredState = &state;
    }

    void setNumPlConstraintsDisabledByValidSplits( unsigned /* numConstraints */ )
    {
    }

    unsigned _timeToSolve;
    IEngine::ExitCode _exitCode;
    bool solve( double timeoutInSeconds )
    {
        if ( timeoutInSeconds >= _timeToSolve )
            _exitCode = IEngine::TIMEOUT;
        return _exitCode == IEngine::SAT;
    }

    void setTimeToSolve( unsigned timeToSolve )
    {
        _timeToSolve = timeToSolve;
    }

    void setExitCode( IEngine::ExitCode exitCode )
    {
        _exitCode = exitCode;
    }

    IEngine::ExitCode getExitCode() const
    {
        return _exitCode;
    }

    void reset()
    {
    }

    List<unsigned> _inputVariables;
    void setInputVariables( List<unsigned> &inputVariables )
    {
        _inputVariables = inputVariables;
    }

    List<unsigned> getInputVariables() const
    {
        return _inputVariables;
    }

    void updateScores( DivideStrategy /**/ )
    {
    }

    mutable SmtState *lastRestoredSmtState;
    bool restoreSmtState( SmtState &smtState )
    {
        lastRestoredSmtState = &smtState;
        return true;
    }

    mutable SmtState *lastStoredSmtState;
    void storeSmtState( SmtState &smtState )
    {
        lastStoredSmtState = &smtState;
    }

    List<PiecewiseLinearConstraint *> _constraintsToSplit;
    void setSplitPLConstraint( PiecewiseLinearConstraint *constraint )
    {
        _constraintsToSplit.append( constraint );
    }

    PiecewiseLinearConstraint *pickSplitPLConstraint( DivideStrategy /**/ )
    {
        if ( !_constraintsToSplit.empty() )
        {
            PiecewiseLinearConstraint *ptr = *_constraintsToSplit.begin();
            _constraintsToSplit.erase( ptr );
            return ptr;
        }
        else
            return NULL;
    }

    PiecewiseLinearConstraint *pickSplitPLConstraintSnC( SnCDivideStrategy /**/ )
    {
        if ( !_constraintsToSplit.empty() )
        {
            PiecewiseLinearConstraint *ptr = *_constraintsToSplit.begin();
            _constraintsToSplit.erase( ptr );
            return ptr;
        }
        else
            return NULL;
    }

    bool _snc;
    CVC4::context::Context _context;

    void applySnCSplit( PiecewiseLinearCaseSplit /*split*/, String /*queryId*/ )
    {
        _snc = true;
        _context.push();
    }

    bool inSnCMode() const
    {
        return _snc;
    }

    void applyAllBoundTightenings(){};

    bool applyAllValidConstraintCaseSplits()
    {
        return false;
    };

    CVC4::context::Context &getContext()
    {
        return _context;
    }

    bool consistentBounds() const
    {
        return true;
    }

    double explainBound( unsigned /* var */, bool /* isUpper */ ) const
    {
        return 0.0;
    }

    void updateGroundUpperBound( unsigned /* var */, double /* value */ )
    {
    }

    void updateGroundLowerBound( unsigned /*var*/, double /*value*/ )
    {
    }

    double getGroundBound( unsigned /*var*/, bool /*isUpper*/ ) const
    {
        return 0;
    }

    UnsatCertificateNode *getUNSATCertificateCurrentPointer() const
    {
        return NULL;
    }

    void setUNSATCertificateCurrentPointer( UnsatCertificateNode * /* node*/ )
    {
    }

    const UnsatCertificateNode *getUNSATCertificateRoot() const
    {
        return NULL;
    }

    bool certifyUNSATCertificate()
    {
        return true;
    }

    void explainSimplexFailure()
    {
    }

    const BoundExplainer *getBoundExplainer() const
    {
        return NULL;
    }

    void setBoundExplainerContent( BoundExplainer * /*boundExplainer */ )
    {
    }

    void propagateBoundManagerTightenings()
    {
    }

    bool shouldProduceProofs() const
    {
        return true;
    }

    void addPLCLemma( std::shared_ptr<PLCLemma> & /*explanation*/ )
    {
    }
};

#endif // __MockEngine_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
