/*********************                                                        */
/*! \file MockConstraintBoundTightenerFactory.h
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

#ifndef __MockConstraintBoundTightenerFactory_h__
#define __MockConstraintBoundTightenerFactory_h__

#include "MockConstraintBoundTightener.h"
#include "T/ConstraintBoundTightenerFactory.h"

class MockConstraintBoundTightenerFactory :
	public T::Base_createConstraintBoundTightener,
	public T::Base_discardConstraintBoundTightener
{
public:
	MockConstraintBoundTightener mockConstraintBoundTightener;

	~MockConstraintBoundTightenerFactory()
	{
		if ( mockConstraintBoundTightener.wasCreated )
		{
			TS_ASSERT( mockConstraintBoundTightener.wasDiscarded );
		}
	}

	IConstraintBoundTightener *createConstraintBoundTightener( const ITableau &tableau )
	{
		mockConstraintBoundTightener.mockConstructor( tableau );
		return &mockConstraintBoundTightener;
	}

	void discardConstraintBoundTightener( IConstraintBoundTightener *constraintBoundTightener )
	{
		TS_ASSERT_EQUALS( constraintBoundTightener, &mockConstraintBoundTightener );
		mockConstraintBoundTightener.mockDestructor();
	}
};

#endif // __MockConstraintBoundTightenerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
