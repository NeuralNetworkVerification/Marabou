/*********************                                                        */
/*! \file Test_CSRMatrix.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "CSRMatrix.h"
#include "MString.h"

class MockForCSRMatrix
{
public:
};

class CSRMatrixTestSuite : public CxxTest::TestSuite
{
public:
    MockForCSRMatrix *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForCSRMatrix );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_sanity()
    {
        /*
          Textbook example: initialize the sparse matrix from
          a dense matrix, and see that the translation is done
          correctly.
        */

        // Initialize through empty constructor and initialize();
        double M1[] = {
                0, 0, 0, 0,
                5, 8, 0, 0,
                0, 0, 3, 0,
                0, 6, 0, 0,
            };

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );
        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( M1[i*4 + j], csr1.get( i, j ) );

        // Dense matrix, initialize through constructor
        double M2[] = {
                1, 2, 3, 4,
                5, 8, 5, 6,
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 1, 2, 3,
            };

        CSRMatrix csr2;
        csr2.initialize( M2, 5, 4 );

        csr2.dump();

        for ( unsigned i = 0; i < 5; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( M2[i*4 + j], csr2.get( i, j ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
