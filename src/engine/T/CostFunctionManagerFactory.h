/*********************                                                        */
/*! \file CostFunctionManagerFactory.h
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

#ifndef __T__CostFunctionManagerFactory_h__
#define __T__CostFunctionManagerFactory_h__

#include "cxxtest/Mock.h"

class ICostFunctionManager;
class ITableau;

namespace T
{
	ICostFunctionManager *createCostFunctionManager( ITableau *tableau );
	void discardCostFunctionManager( ICostFunctionManager *costFunctionManager );
}

CXXTEST_SUPPLY( createCostFunctionManager,
				ICostFunctionManager *,
				createCostFunctionManager,
				( ITableau *tableau ),
				T::createCostFunctionManager,
				( tableau ) );

CXXTEST_SUPPLY_VOID( discardCostFunctionManager,
					 discardCostFunctionManager,
					 ( ICostFunctionManager *costFunctionManager ),
					 T::discardCostFunctionManager,
					 ( costFunctionManager ) );

#endif // __T__CostFunctionManagerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
