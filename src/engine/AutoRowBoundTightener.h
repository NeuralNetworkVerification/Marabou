/*********************                                                        */
/*! \file AutoRowBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AutoRowBoundTightener_h__
#define __AutoRowBoundTightener_h__

#include "IRowBoundTightener.h"
#include "T/RowBoundTightenerFactory.h"

class AutoRowBoundTightener
{
public:
	AutoRowBoundTightener( const ITableau &tableau )
	{
		_rowBoundTightener = T::createRowBoundTightener( tableau );
	}

	~AutoRowBoundTightener()
	{
		T::discardRowBoundTightener( _rowBoundTightener );
		_rowBoundTightener = 0;
	}

	operator IRowBoundTightener &()
	{
		return *_rowBoundTightener;
	}

	operator IRowBoundTightener *()
	{
		return _rowBoundTightener;
	}

	IRowBoundTightener *operator->()
	{
		return _rowBoundTightener;
	}

	const IRowBoundTightener *operator->() const
	{
		return _rowBoundTightener;
	}

private:
	IRowBoundTightener *_rowBoundTightener;
};

#endif // __AutoRowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
