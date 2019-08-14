/*********************                                                        */
/*! \file AutoProjectedSteepestEdge.h
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

#ifndef __AutoProjectedSteepestEdge_h__
#define __AutoProjectedSteepestEdge_h__

#include "IProjectedSteepestEdge.h"
#include "T/ProjectedSteepestEdgeFactory.h"

class AutoProjectedSteepestEdgeRule
{
public:
	AutoProjectedSteepestEdgeRule()
	{
		_projectedSteepestEdgeRule = T::createProjectedSteepestEdgeRule();
	}

	~AutoProjectedSteepestEdgeRule()
	{
		T::discardProjectedSteepestEdgeRule( _projectedSteepestEdgeRule );
		_projectedSteepestEdgeRule = 0;
	}

	operator IProjectedSteepestEdgeRule &()
	{
		return *_projectedSteepestEdgeRule;
	}

	operator IProjectedSteepestEdgeRule *()
	{
		return _projectedSteepestEdgeRule;
	}

	IProjectedSteepestEdgeRule *operator->()
	{
		return _projectedSteepestEdgeRule;
	}

	const IProjectedSteepestEdgeRule *operator->() const
	{
		return _projectedSteepestEdgeRule;
	}

private:
	IProjectedSteepestEdgeRule *_projectedSteepestEdgeRule;
};

#endif // __AutoProjectedSteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
