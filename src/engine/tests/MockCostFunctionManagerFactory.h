/*********************                                                        */
/*! \file MockCostFunctionManagerFactory.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockCostFunctionManagerFactory_h__
#define __MockCostFunctionManagerFactory_h__

#include "MockCostFunctionManager.h"
#include "T/CostFunctionManagerFactory.h"

class MockCostFunctionManagerFactory :
	public T::Base_createCostFunctionManager,
	public T::Base_discardCostFunctionManager
{
public:
	MockCostFunctionManager mockCostFunctionManager;

	~MockCostFunctionManagerFactory()
	{
		if ( mockCostFunctionManager.wasCreated )
		{
			TS_ASSERT( mockCostFunctionManager.wasDiscarded );
		}
	}

	ICostFunctionManager *createCostFunctionManager( ITableau *tableau )
	{
		mockCostFunctionManager.mockConstructor( tableau );
		return &mockCostFunctionManager;
	}

	void discardCostFunctionManager( ICostFunctionManager *costFunctionManager )
	{
		TS_ASSERT_EQUALS( costFunctionManager, &mockCostFunctionManager );
		mockCostFunctionManager.mockDestructor();
	}
};

#endif // __MockCostFunctionManagerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
