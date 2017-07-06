/*********************                                                        */
/*! \file MockEntrySelectionStrategy.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockEntrySelectionStrategy_h__
#define __MockEntrySelectionStrategy_h__

#include "EntrySelectionStrategy.h"
#include "ITableau.h"

class MockEntrySelectionStrategy : public EntrySelectionStrategy
{
public:
	MockEntrySelectionStrategy()
	{
		wasCreated = false;
		wasDiscarded = false;
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

    List<unsigned> lastCandidates;
    unsigned nextSelectResult;

    bool select( ITableau & tableau )
    {
    	lastCandidates.clear();
        tableau.getCandidates( lastCandidates );
        if (!lastCandidates.empty()) {
	    	tableau.setEnteringVariable(nextSelectResult);
        }
        return !lastCandidates.empty();
    }
};

#endif // __MockEntrySelectionStrategy_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
