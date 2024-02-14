/*********************                                                        */
/*! \file AutoConstraintMatrixAnalyzer.h
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

#ifndef __AutoConstraintMatrixAnalyzer_h__
#define __AutoConstraintMatrixAnalyzer_h__

#include "IConstraintMatrixAnalyzer.h"
#include "T/ConstraintMatrixAnalyzerFactory.h"

class AutoConstraintMatrixAnalyzer
{
public:
	AutoConstraintMatrixAnalyzer()
	{
		_constraintMatrixAnalyzer = T::createConstraintMatrixAnalyzer();
	}

	~AutoConstraintMatrixAnalyzer()
	{
		T::discardConstraintMatrixAnalyzer( _constraintMatrixAnalyzer );
	}

	operator IConstraintMatrixAnalyzer &()
	{
		return *_constraintMatrixAnalyzer;
	}

	operator IConstraintMatrixAnalyzer *()
	{
		return _constraintMatrixAnalyzer;
	}

	IConstraintMatrixAnalyzer *operator->()
	{
		return _constraintMatrixAnalyzer;
	}

	const IConstraintMatrixAnalyzer *operator->() const
	{
		return _constraintMatrixAnalyzer;
	}

private:
	IConstraintMatrixAnalyzer *_constraintMatrixAnalyzer;
};

#endif // __AutoConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
