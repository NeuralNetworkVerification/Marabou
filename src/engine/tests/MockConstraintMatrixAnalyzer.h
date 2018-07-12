/*********************                                                        */
/*! \file MockConstraintMatrixAnalyzer.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockConstraintMatrixAnalyzer_h__
#define __MockConstraintMatrixAnalyzer_h__

#include "IConstraintMatrixAnalyzer.h"

class MockConstraintMatrixAnalyzer : public IConstraintMatrixAnalyzer
{
public:
	MockConstraintMatrixAnalyzer()
	{
		wasCreated = false;
		wasDiscarded = false;
    }

    ~MockConstraintMatrixAnalyzer()
    {
    }

	bool wasCreated;
	bool wasDiscarded;

	void mockConstructor()
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}

    void analyze( const double */* matrix */, unsigned /* m */, unsigned /* n */ )
    {
    }

    void analyze( const SparseMatrix */* matrix */, unsigned /* m */, unsigned /* n */ )
    {
    }

    unsigned getRank() const
    {
        return 0;
    }

    List<unsigned> nextIndependentColumns;

    List<unsigned> getIndependentColumns() const
    {
        return nextIndependentColumns;
    }
};

#endif // __MockConstraintMatrixAnalyzer_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
