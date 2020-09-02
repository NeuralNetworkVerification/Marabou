/*********************                                                        */
/*! \file Test_ConstraintMatrixAnalyzer.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

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

    bool sameColumns( List<unsigned> a, Set<unsigned> b )
    {
        if ( a.size() != b.size() )
            return false;

        for ( const auto &it : a )
            if ( !b.exists( it ) )
                return false;

        return true;
    }

    void test_analyze__gaussian_eliminiation()
    {
        ConstraintMatrixAnalyzer analyzer;

        {
            double A1[] = {
                1, 0, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 0, 0, 1, 0,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT( analyzer.getRedundantRows().empty() );
            Set<unsigned> expectedColumns( { 0, 2, 3 } );
            TS_ASSERT( sameColumns( analyzer.getIndependentColumns(), expectedColumns ) );
        }

        {
            double A1[] = {
                1, 0, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 1, 0, 1, 0,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT( analyzer.getRedundantRows().empty() );

            Set<unsigned> expectedColumns1( { 0, 1, 2 } );
            Set<unsigned> expectedColumns2( { 0, 2, 3 } );
            TS_ASSERT( sameColumns( analyzer.getIndependentColumns(), expectedColumns1 )
                       ||
                       sameColumns( analyzer.getIndependentColumns(), expectedColumns2 )
                       );
        }

        {
            double A1[] = {
                1, 0, 0, 0, 0,
                1, 0, 0, 0, 0,
                0, 1, 0, 2, 0,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            Set<unsigned> expectedRows( { 1 } );
            TS_ASSERT_EQUALS( analyzer.getRedundantRows(), expectedRows );

            Set<unsigned> expectedColumns1( { 0, 1 } );
            Set<unsigned> expectedColumns2( { 0, 3 } );
            TS_ASSERT( sameColumns( analyzer.getIndependentColumns(), expectedColumns1 )
                       ||
                       sameColumns( analyzer.getIndependentColumns(), expectedColumns2 )
                       );
        }

        {
            double A1[] = {
                1, 1, 0, 1, 0,
                0, 0, 3, 0, 0,
                2, 2, 0, 0, 0,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT( analyzer.getRedundantRows().empty() );

            Set<unsigned> expectedColumns1( { 0, 2, 3 } );
            Set<unsigned> expectedColumns2( { 1, 2, 3 } );
            TS_ASSERT( sameColumns( analyzer.getIndependentColumns(), expectedColumns1 )
                       ||
                       sameColumns( analyzer.getIndependentColumns(), expectedColumns2 )
                       );
        }

        {
            double A1[] = {
                15, 3,  0, 1, 0,
                0 , 0, -1, 1, 4,
                15, 3, -1, 2, 4,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT_EQUALS( analyzer.getRedundantRows().size(), 1U );

            List<unsigned> columns = analyzer.getIndependentColumns();
            TS_ASSERT_EQUALS( columns.size(), 2U );

            // Can have at most 1 of columns 0 and 1
            TS_ASSERT( !columns.exists( 0 ) || !columns.exists( 1 ) );

            // Can have at most 1 of columns 2 and 4
            TS_ASSERT( !columns.exists( 2 ) || !columns.exists( 4 ) );
        }

        {
            double A1[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT( analyzer.getIndependentColumns().empty() );

            TS_ASSERT_EQUALS( analyzer.getRedundantRows().size(), 3U );
        }

        {
            double A1[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 2, 3, 14, 1,
            };

            TS_ASSERT_THROWS_NOTHING( analyzer.analyze( A1, 3, 5 ) );

            TS_ASSERT_EQUALS( analyzer.getRedundantRows(),
                              Set<unsigned>( { 0, 1 } ) );

            List<unsigned> columns = analyzer.getIndependentColumns();
            TS_ASSERT_EQUALS( columns.size(), 1U );
            TS_ASSERT( !columns.exists( 0 ) );
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
