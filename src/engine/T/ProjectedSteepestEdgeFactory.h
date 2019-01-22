/*********************                                                        */
/*! \file ProjectedSteepestEdgeFactory.h
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

#ifndef __T__ProjectedSteepestEdgeFactory_h__
#define __T__ProjectedSteepestEdgeFactory_h__

#include "cxxtest/Mock.h"

class IProjectedSteepestEdgeRule;

namespace T
{
	IProjectedSteepestEdgeRule *createProjectedSteepestEdgeRule();
	void discardProjectedSteepestEdgeRule( IProjectedSteepestEdgeRule *projectedSteepestEdgeRule );
}

CXXTEST_SUPPLY( createProjectedSteepestEdgeRule,
				IProjectedSteepestEdgeRule *,
				createProjectedSteepestEdgeRule,
				(),
				T::createProjectedSteepestEdgeRule,
				() );

CXXTEST_SUPPLY_VOID( discardProjectedSteepestEdgeRule,
					 discardProjectedSteepestEdgeRule,
					 ( IProjectedSteepestEdgeRule *projectedSteepestEdgeRule ),
					 T::discardProjectedSteepestEdgeRule,
					 ( projectedSteepestEdgeRule ) );

#endif // __T__ProjectedSteepestEdgeFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
