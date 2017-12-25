/*********************                                                        */
/*! \file MockCostFunctionManager.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockCostFunctionManager_h__
#define __MockCostFunctionManager_h__

#include "ICostFunctionManager.h"

#include <cstring>

class MockCostFunctionManager : public ICostFunctionManager
{
public:
	MockCostFunctionManager()
	{
		wasCreated = false;
		wasDiscarded = false;

        initializeWasCalled = false;
        lastTableau = NULL;
    }

    ~MockCostFunctionManager()
    {
    }

	bool wasCreated;
	bool wasDiscarded;

    ITableau *lastTableau;
	void mockConstructor( ITableau *tableau )
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;

        lastTableau = tableau;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}

    bool initializeWasCalled;
    void initialize()
    {
        initializeWasCalled = true;
    }
};

#endif // __MockCostFunctionManager_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
