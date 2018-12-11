/*********************                                                        */
/*! \file AutoConstraintBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AutoConstraintBoundTightener_h__
#define __AutoConstraintBoundTightener_h__

#include "IConstraintBoundTightener.h"
#include "T/ConstraintBoundTightenerFactory.h"

class AutoConstraintBoundTightener
{
public:
	AutoConstraintBoundTightener( const ITableau &tableau )
	{
		_constraintBoundTightener = T::createConstraintBoundTightener( tableau );
	}

	~AutoConstraintBoundTightener()
	{
		T::discardConstraintBoundTightener( _constraintBoundTightener );
		_constraintBoundTightener = 0;
	}

	operator IConstraintBoundTightener &()
	{
		return *_constraintBoundTightener;
	}

	operator IConstraintBoundTightener *()
	{
		return _constraintBoundTightener;
	}

	IConstraintBoundTightener *get()
    {
        return _constraintBoundTightener;
    }

	IConstraintBoundTightener *operator->()
	{
		return _constraintBoundTightener;
	}

	const IConstraintBoundTightener *operator->() const
	{
		return _constraintBoundTightener;
	}

private:
	IConstraintBoundTightener *_constraintBoundTightener;
};

#endif // __AutoConstraintBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
