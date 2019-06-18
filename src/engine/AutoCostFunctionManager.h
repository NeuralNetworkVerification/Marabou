/*********************                                                        */
/*! \file AutoCostFunctionManager.h
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

#ifndef __AutoCostFunctionManager_h__
#define __AutoCostFunctionManager_h__

#include "ICostFunctionManager.h"
#include "T/CostFunctionManagerFactory.h"

class AutoCostFunctionManager
{
public:
	AutoCostFunctionManager( ITableau *tableau )
	{
		_costFunctionManager = T::createCostFunctionManager( tableau );
	}

    AutoCostFunctionManager( ITableau *tableau , double *costFunction)
    {
        _costFunctionManager = T::createCostFunctionManager( tableau, costFunction );
    }

	~AutoCostFunctionManager()
	{
		T::discardCostFunctionManager( _costFunctionManager );
		_costFunctionManager = 0;
	}

	operator ICostFunctionManager &()
	{
		return *_costFunctionManager;
	}

	operator ICostFunctionManager *()
	{
		return _costFunctionManager;
	}

	ICostFunctionManager *operator->()
	{
		return _costFunctionManager;
	}

	const ICostFunctionManager *operator->() const
	{
		return _costFunctionManager;
	}

private:
	ICostFunctionManager *_costFunctionManager;
};

#endif // __AutoCostFunctionManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
