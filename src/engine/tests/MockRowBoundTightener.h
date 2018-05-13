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

        initializeWasCalled = false;
    }

    ~MockRowBoundTightener()
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

    bool initializeWasCalled;
    void initialize( const ITableau &/* tableau */ )
    {
        initializeWasCalled = true;
    }

    void clear( const ITableau &/* tableau */ ) {}
    void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}
    void examineBasisMatrix( const ITableau &/* tableau */, bool /* untilSaturation */ ) {}
    void examineInvertedBasisMatrix( const ITableau &/* tableau */, bool /* untilSaturation */ ) {}
    void examineConstraintMatrix( const ITableau &/* tableau */, bool /* untilSaturation */ ) {}
    void examinePivotRow( ITableau &/* tableau */ ) {}
    void getRowTightenings( List<Tightening> &/* tightenings */ ) const {}
    void setStatistics( Statistics */* statistics */ ) {}

    void explicitBasisBoundTightening( const ITableau &/* tableau */ ) {}
};

#endif // __MockRowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
