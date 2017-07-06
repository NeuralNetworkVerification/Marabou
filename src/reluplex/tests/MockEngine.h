/*********************                                                        */
/*! \file MockEngine.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockEngine_h__
#define __MockEngine_h__

#include "IEngine.h"
#include "List.h"

class MockEngine : public IEngine
{
public:
	MockEngine()
	{
		wasCreated = false;
		wasDiscarded = false;
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
    void tightenLowerBound( unsigned variable, double bound )
    {
        lastLowerBounds.append( Bound( variable, bound ) );
    }

    List<Bound> lastUpperBounds;
    void tightenUpperBound( unsigned variable, double bound )
    {
        lastUpperBounds.append( Bound( variable, bound ) );
    }

    List<Equation> lastEquations;
    void addNewEquation( const Equation &equation )
    {
        lastEquations.append( equation );
    }

    void storeTableauState( TableauState &// state
                            ) const
    {
    }

    void restoreTableauState( const TableauState &// state
                              )
    {
    }
};

#endif // __MockEngine_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
