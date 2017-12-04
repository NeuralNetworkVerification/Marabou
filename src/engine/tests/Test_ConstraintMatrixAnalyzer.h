 /*********************                                                        */
/*! \file Test_ConstraintMatrixAnalyzer.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim **/

#include <cxxtest/TestSuite.h>

#include "ConstraintMatrixAnalyzer.h"

#include <string.h>
#include <cstdio>

class MockForConstraintMatrixAnalyzer
{
public:
};

class ConstraintMatrixAnalyzerTestSuite : public CxxTest::TestSuite
{
public:
	MockForConstraintMatrixAnalyzer *mock;

	void setUp()
	{
		TS_ASSERT( mock = new MockForConstraintMatrixAnalyzer );
	}

	void tearDown()
	{
		TS_ASSERT_THROWS_NOTHING( delete mock );
	}

    void transpose( double *matrix, unsigned m, unsigned n )
    {
        double *temp = new double[m*n];
        for ( unsigned i = 0; i < m; ++i )
            for ( unsigned j = 0; j < n; ++j )
                temp[j*m + i] = matrix[i*n + j];

        memcpy( matrix, temp, sizeof(double) * m * n );
        delete[] temp;
    }

    void test_analyze__gaussian_eliminiation()
    {
        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                1, 0, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 0, 0, 1, 0,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), A1, sizeof(A1) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 3U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                1, 0, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 1, 0, 1, 0,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                1, 0, 0, 0, 0,
                0, 1, 0, 1, 0,
                0, 0, 1, 0, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 3U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                1, 0, 0, 0, 0,
                1, 0, 0, 0, 0,
                0, 1, 0, 2, 0,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                1, 0, 0, 0, 0,
                0, 1, 0, 2, 0,
                0, 0, 0, 0, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 2U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                1, 1, 0, 1, 0,
                0, 0, 3, 0, 0,
                2, 2, 0, 0, 0,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                2, 2, 0, 0, 0,
                0, 0, 3, 0, 0,
                0, 0, 0, 1, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 3U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                15, 3,  0, 1, 0,
                0 , 0, -1, 1, 4,
                15, 3, -1, 2, 4,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                15, 3,  0, 1, 0,
                0,  0, -1, 1, 4,
                0,  0,  0, 0, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 2U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 0U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }

        {
            ConstraintMatrixAnalyzer *analyzer;
            TS_ASSERT( analyzer = new ConstraintMatrixAnalyzer );

            double A1[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 2, 3, 14, 1,
            };

            double A1t[sizeof(A1) / sizeof(double)];
            memcpy( A1t, A1, sizeof(A1) );
            transpose( A1t, 3, 5 );

            TS_ASSERT_THROWS_NOTHING( analyzer->analyze( A1t, 3, 5 ) );

            double expectedResult[] = {
                0, 2, 3, 14, 1,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            TS_ASSERT_SAME_DATA( analyzer->getCanonicalForm(), expectedResult, sizeof(expectedResult) );
            TS_ASSERT_EQUALS( analyzer->getRank(), 1U );

            TS_ASSERT_THROWS_NOTHING( delete analyzer );
        }
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
