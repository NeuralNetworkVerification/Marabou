/*********************                                                        */
/*! \file MockRowBoundTightenerFactory.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockRowBoundTightenerFactory_h__
#define __MockRowBoundTightenerFactory_h__

#include "MockRowBoundTightener.h"
#include "T/RowBoundTightenerFactory.h"

class MockRowBoundTightenerFactory :
	public T::Base_createRowBoundTightener,
	public T::Base_discardRowBoundTightener
{
public:
	MockRowBoundTightener mockRowBoundTightener;

	~MockRowBoundTightenerFactory()
	{
		if ( mockRowBoundTightener.wasCreated )
		{
			TS_ASSERT( mockRowBoundTightener.wasDiscarded );
		}
	}

	IRowBoundTightener *createRowBoundTightener()
	{
		mockRowBoundTightener.mockConstructor();
		return &mockRowBoundTightener;
	}

	void discardRowBoundTightener( IRowBoundTightener *rowBoundTightener )
	{
		TS_ASSERT_EQUALS( rowBoundTightener, &mockRowBoundTightener );
		mockRowBoundTightener.mockDestructor();
	}
};

#endif // __MockRowBoundTightenerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
