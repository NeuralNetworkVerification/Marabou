/*********************                                                        */
/*! \file ConstraintMatrixAnalyzerFactory.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __T__ConstraintMatrixAnalyzerFactory_h__
#define __T__ConstraintMatrixAnalyzerFactory_h__

#include "cxxtest/Mock.h"

class IConstraintMatrixAnalyzer;
class ISelector;

namespace T
{
	IConstraintMatrixAnalyzer *createConstraintMatrixAnalyzer();
	void discardConstraintMatrixAnalyzer( IConstraintMatrixAnalyzer *constraintMatrixAnalyzer );
}

CXXTEST_SUPPLY( createConstraintMatrixAnalyzer,
				IConstraintMatrixAnalyzer *,
				createConstraintMatrixAnalyzer,
				(),
				T::createConstraintMatrixAnalyzer,
				() );

CXXTEST_SUPPLY_VOID( discardConstraintMatrixAnalyzer,
					 discardConstraintMatrixAnalyzer,
					 ( IConstraintMatrixAnalyzer *constraintMatrixAnalyzer ),
					 T::discardConstraintMatrixAnalyzer,
					 ( constraintMatrixAnalyzer ) );

#endif // __T__ConstraintMatrixAnalyzerFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
