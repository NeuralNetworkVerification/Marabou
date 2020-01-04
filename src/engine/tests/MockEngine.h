/*********************                                                        */
/*! \file MockEngine.h
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

#ifndef __MockEngine_h__
#define __MockEngine_h__

#include "IEngine.h"
#include "List.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"

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

    mutable EngineState *lastStoredState;
    void storeState( EngineState &state, bool /* storeAlsoTableauState */ ) const
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
    bool solve( unsigned timeoutInSeconds )
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

    bool propagate()
    {
        return true;
    }

    void getEstimates( Map <unsigned, double>
                       &balanceEstimates,
                       Map <unsigned, double>
                       &runtimeEstimates )
    {
        balanceEstimates.clear();
        runtimeEstimates.clear();
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

    PiecewiseLinearConstraint *getConstraintFromId( unsigned id )
    {
        std::cout << id << std::endl;
        return NULL;
    }

    void setBiasedPhases( unsigned biasedLayer, BiasStrategy strategy )
    {
        std::cout << biasedLayer << " " << strategy << std::endl;
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
