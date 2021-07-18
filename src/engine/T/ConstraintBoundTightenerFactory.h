/*********************                                                        */
/*! \file ConstraintBoundTightenerFactory.h
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

#ifndef __T__ConstraintBoundTightenerFactory_h__
#define __T__ConstraintBoundTightenerFactory_h__

#include "cxxtest/Mock.h"

class IConstraintBoundTightener;
class ITableau;
class IEngine;

namespace T
{
	IConstraintBoundTightener *createConstraintBoundTightener( ITableau &tableau, IEngine &engine );
	void discardConstraintBoundTightener( IConstraintBoundTightener *constraintBoundTightener );
}

CXXTEST_SUPPLY( createConstraintBoundTightener,
				IConstraintBoundTightener *,
				createConstraintBoundTightener,
				( ITableau &tableau, IEngine &engine ),
				T::createConstraintBoundTightener,
				( tableau, engine ) );

CXXTEST_SUPPLY_VOID( discardConstraintBoundTightener,
					 discardConstraintBoundTightener,
					 ( IConstraintBoundTightener *constraintBoundTightener ),
					 T::discardConstraintBoundTightener,
					 ( constraintBoundTightener ) );

#endif // __T__ConstraintBoundTightenerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
