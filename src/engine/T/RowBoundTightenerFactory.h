/*********************                                                        */
/*! \file RowBoundTightenerFactory.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __T__RowBoundTightenerFactory_h__
#define __T__RowBoundTightenerFactory_h__

#include "cxxtest/Mock.h"

class IRowBoundTightener;
class ITableau;

namespace T
{
	IRowBoundTightener *createRowBoundTightener( const ITableau &tableau );
	void discardRowBoundTightener( IRowBoundTightener *rowBoundTightener );
}

CXXTEST_SUPPLY( createRowBoundTightener,
				IRowBoundTightener *,
				createRowBoundTightener,
				( const ITableau &tableau ),
				T::createRowBoundTightener,
				( tableau ) );

CXXTEST_SUPPLY_VOID( discardRowBoundTightener,
					 discardRowBoundTightener,
					 ( IRowBoundTightener *rowBoundTightener ),
					 T::discardRowBoundTightener,
					 ( rowBoundTightener ) );

#endif // __T__RowBoundTightenerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
