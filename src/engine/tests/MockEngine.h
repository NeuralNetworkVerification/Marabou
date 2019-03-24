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
#include "FactTracker.h"
#include "List.h"
#include "PiecewiseLinearCaseSplit.h"

class MockEngine : public IEngine
{
public:
    MockEngine()
    {
        wasCreated = false;
        wasDiscarded = false;
        factTracker = NULL;
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
    FactTracker* factTracker;
    void setFactTracker(FactTracker* f)
    {
      factTracker = f;
    }
    void applySplit( const PiecewiseLinearCaseSplit &split )
    {
        List<Tightening> bounds = split.getBoundTightenings();
        auto equations = split.getEquations();
        unsigned i=1;
        for ( auto &it : equations )
        {
            lastEquations.append( it );
            if(factTracker){
              factTracker->addEquationFact(i, it);
              i++;
            }
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
            if(factTracker){
              factTracker->addBoundFact(bound._variable, bound);
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

};

#endif // __MockEngine_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//