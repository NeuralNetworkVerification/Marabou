/*********************                                                        */
/*! \file MockConstraintBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __MockConstraintBoundTightener_h__
#define __MockConstraintBoundTightener_h__

#include "IConstraintBoundTightener.h"

class MockConstraintBoundTightener : public IConstraintBoundTightener
{
public:
	MockConstraintBoundTightener()
	{
		wasCreated = false;
		wasDiscarded = false;

        setDimensionsWasCalled = false;
    }

    ~MockConstraintBoundTightener()
    {
    }

	bool wasCreated;
	bool wasDiscarded;
    const ITableau *lastTableau;

	void mockConstructor( const ITableau &tableau )
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;
        lastTableau = &tableau;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}

    bool setDimensionsWasCalled;
    void setDimensions()
    {
        setDimensionsWasCalled = true;
    }

    void resetBounds() {}
    void notifyDimensionChange( unsigned /* m */, unsigned /* n */ ) {}
    void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}
    void setStatistics( Statistics */* statistics */ ) {}
    void registerTighterLowerBound( unsigned /* variable */, double /* bound */, const Fact* /* explanationID */ ) {}
    void registerTighterUpperBound( unsigned /* variable */, double /* bound */, const Fact* /* explanationID */ ) {}
    void getConstraintTightenings( List<Tightening> &/* tightenings */ ) const {}
    void setFactTracker( FactTracker* /* factTracker */ ) {};
};

#endif // __MockConstraintBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
