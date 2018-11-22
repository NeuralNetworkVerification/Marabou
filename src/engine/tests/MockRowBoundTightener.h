/*********************                                                        */
/*! \file MockRowBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockRowBoundTightener_h__
#define __MockRowBoundTightener_h__

#include "IRowBoundTightener.h"

class MockRowBoundTightener : public IRowBoundTightener
{
public:
	MockRowBoundTightener()
	{
		wasCreated = false;
		wasDiscarded = false;

        setDimensionsWasCalled = false;
    }

    ~MockRowBoundTightener()
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

    void resetBounds()
    {
    }

    void clear() {}
    void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}
    void examineInvertedBasisMatrix( bool /* untilSaturation */ ) {}
    void examineConstraintMatrix( bool /* untilSaturation */ ) {}
    void examinePivotRow() {}
    void getRowTightenings( List<Tightening> &/* tightenings */ ) const {}
    void setStatistics( Statistics */* statistics */ ) {}
    void examineImplicitInvertedBasisMatrix( bool /* untilSaturation */ ) {}
		SplitSet getNewSplitsAffectingVariable( unsigned /* var */ ) const { return SplitSet();}
};

#endif // __MockRowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
