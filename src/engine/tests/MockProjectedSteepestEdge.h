/*********************                                                        */
/*! \file MockProjectedSteepestEdge.h
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

#ifndef __MockProjectedSteepestEdge_h__
#define __MockProjectedSteepestEdge_h__

#include "IProjectedSteepestEdge.h"

class MockProjectedSteepestEdgeRule : public IProjectedSteepestEdgeRule
{
public:
	MockProjectedSteepestEdgeRule()
	{
		wasCreated = false;
		wasDiscarded = false;
    }

    ~MockProjectedSteepestEdgeRule()
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

    void initialize( const ITableau & )
    {
    }

    bool select( ITableau &,
                 const List<unsigned> &,
                 const Set<unsigned> & )
    {
        return true;
    }

    void prePivotHook( const ITableau &, bool )
    {
    }

    void postPivotHook( const ITableau &, bool )
    {
    }

    void resizeHook( const ITableau & )
    {
    }

    double getGamma( unsigned ) const
    {
        return 0;
    }
};

#endif // __MockProjectedSteepestEdgeRule_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
