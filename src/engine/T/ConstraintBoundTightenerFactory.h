/*********************                                                        */
/*! \file ConstraintBoundTightenerFactory.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __T__ConstraintBoundTightenerFactory_h__
#define __T__ConstraintBoundTightenerFactory_h__

#include "cxxtest/Mock.h"

class IConstraintBoundTightener;
class ITableau;

namespace T
{
	IConstraintBoundTightener *createConstraintBoundTightener( const ITableau &tableau );
	void discardConstraintBoundTightener( IConstraintBoundTightener *constraintBoundTightener );
}

CXXTEST_SUPPLY( createConstraintBoundTightener,
				IConstraintBoundTightener *,
				createConstraintBoundTightener,
				( const ITableau &tableau ),
				T::createConstraintBoundTightener,
				( tableau ) );

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
