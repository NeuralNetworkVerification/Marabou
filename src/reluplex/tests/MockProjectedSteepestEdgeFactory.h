/*********************                                                        */
/*! \file MockProjectedSteepestEdgeFactory.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockProjectedSteepestEdgeFactory_h__
#define __MockProjectedSteepestEdgeFactory_h__

#include "MockProjectedSteepestEdge.h"
#include "T/ProjectedSteepestEdgeFactory.h"

class MockProjectedSteepestEdgeRuleFactory :
	public T::Base_createProjectedSteepestEdgeRule,
	public T::Base_discardProjectedSteepestEdgeRule
{
public:
	MockProjectedSteepestEdgeRule mockProjectedSteepestEdgeRule;

	~MockProjectedSteepestEdgeRuleFactory()
	{
		if ( mockProjectedSteepestEdgeRule.wasCreated )
		{
			TS_ASSERT( mockProjectedSteepestEdgeRule.wasDiscarded );
		}
	}

	IProjectedSteepestEdgeRule *createProjectedSteepestEdgeRule()
	{
		mockProjectedSteepestEdgeRule.mockConstructor();
		return &mockProjectedSteepestEdgeRule;
	}

	void discardProjectedSteepestEdgeRule( IProjectedSteepestEdgeRule *projectedSteepestEdgeRule )
	{
		TS_ASSERT_EQUALS( projectedSteepestEdgeRule, &mockProjectedSteepestEdgeRule );
		mockProjectedSteepestEdgeRule.mockDestructor();
	}
};

#endif // __MockProjectedSteepestEdgeFactory_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
