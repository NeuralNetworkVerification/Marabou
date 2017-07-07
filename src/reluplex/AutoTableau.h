/*********************                                                        */
/*! \file AutoTableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AutoTableau_h__
#define __AutoTableau_h__

#include "ITableau.h"
#include "T/TableauFactory.h"

class AutoTableau
{
public:
	AutoTableau()
	{
		_tableau = T::createTableau();
	}

	~AutoTableau()
	{
		T::discardTableau( _tableau );
		_tableau = 0;
	}

	operator ITableau &()
	{
		return *_tableau;
	}

	operator ITableau *()
	{
		return _tableau;
	}

	ITableau *operator->()
	{
		return _tableau;
	}

	const ITableau *operator->() const
	{
		return _tableau;
	}

private:
	ITableau *_tableau;
};

#endif // __AutoTableau_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
