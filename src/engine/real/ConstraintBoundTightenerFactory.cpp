/*********************                                                        */
/*! \file ConstraintBoundTightenerFactory.cpp
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

#include "ConstraintBoundTightener.h"

namespace T
{
	IConstraintBoundTightener *createConstraintBoundTightener( const ITableau &tableau )
	{
		return new ConstraintBoundTightener( tableau );
	}

	void discardConstraintBoundTightener( IConstraintBoundTightener *constraintBoundTightener )
	{
		delete constraintBoundTightener;
	}
}

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
