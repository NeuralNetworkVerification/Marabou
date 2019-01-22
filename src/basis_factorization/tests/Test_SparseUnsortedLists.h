/*********************                                                        */
/*! \file Test_SparseUnsortedLists.h
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

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "MString.h"
#include "SparseUnsortedLists.h"

class MockForSparseUnsortedLists
{
public:
};

class SparseUnsortedListsTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseUnsortedLists *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseUnsortedLists );
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

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );
        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( M1[i*4 + j], sv1.get( i, j ) );

        // Dense matrix, initialize through constructor
        double M2[] = {
                1, 2, 3, 4,
                5, 8, 5, 6,
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 1, 2, 3,
            };

        SparseUnsortedLists sv2;
        sv2.initialize( M2, 5, 4 );

        for ( unsigned i = 0; i < 5; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( M2[i*4 + j], sv2.get( i, j ) );
    }

    void test_store_restore()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        SparseUnsortedLists sv2;
        sv1.storeIntoOther( &sv2 );

        SparseUnsortedLists sv3;
        sv2.storeIntoOther( &sv3 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), sv3.get( i, j ) );
    }

    void test_add_last_row()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        double row5[] = { 1, 2, 0, 0 };
        double row6[] = { 0, 2, -3, 0 };
        double row7[] = { 1, 0, 0, 4 };

        sv1.addLastRow( row5 );
        sv1.addLastRow( row6 );
        sv1.addLastRow( row7 );

        double expected[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
            1, 2, 0, 0,
            0, 2, -3, 0,
            1, 0, 0, 4,
        };

        for ( unsigned i = 0; i < 7; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*4 + j] );
    }

    void test_add_last_column()
    {
        {
            double M1[] = {
                0, 0, 0, 2,
                5, 8, 0, 0,
                0, 0, 3, 0,
                0, 6, 0, 0,
            };

            SparseUnsortedLists sv1;
            sv1.initialize( M1, 4, 4 );

            double col5[] = { 1, 2, 0, 0 };
            double col6[] = { 0, 2, -3, 0 };
            double col7[] = { 1, 0, 0, 4 };

            sv1.addLastColumn( col5 );
            sv1.addLastColumn( col6 );
            sv1.addLastColumn( col7 );

            double expected[] = {
                0, 0, 0, 2, 1,  0, 1,
                5, 8, 0, 0, 2,  2, 0,
                0, 0, 3, 0, 0, -3, 0,
                0, 6, 0, 0, 0,  0, 4,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 7; ++j )
                    TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*7 + j] );

            TS_ASSERT_EQUALS( sv1.getNnz(), 11U );
        }

        {
            double M1[] = {
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };

            SparseUnsortedLists sv1;
            sv1.initialize( M1, 4, 4 );

            double col5[] = { 0, 0, 0, 0 };
            sv1.addLastColumn( col5 );

            double expected[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 5; ++j )
                    TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*5 + j] );

            TS_ASSERT_EQUALS( sv1.getNnz(), 0U );
        }
    }

    void test_get_row()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        const SparseUnsortedList *row;
        TS_ASSERT_THROWS_NOTHING( row = sv1.getRow( 1 ) );
        TS_ASSERT_EQUALS( row->getNnz(), 2U );
        TS_ASSERT_EQUALS( row->get( 0 ), 5.0 );
        TS_ASSERT_EQUALS( row->get( 1 ), 8.0 );

        TS_ASSERT_THROWS_NOTHING( row = sv1.getRow( 3 ) );
        TS_ASSERT_EQUALS( row->getNnz(), 1U );
        TS_ASSERT_EQUALS( row->get( 1 ), 6.0 );

        TS_ASSERT_THROWS_NOTHING( row = sv1.getRow( 0 ) );
        TS_ASSERT( row->empty() );
    }

    void test_to_dense()
    {
        {
            double M1[] = {
                0, 0, 0, 0,
                5, 8, 0, 0,
                0, 0, 3, 0,
                0, 6, 0, 0,
            };

            SparseUnsortedLists sv1;
            sv1.initialize( M1, 4, 4 );

            double dense[16];

            TS_ASSERT_THROWS_NOTHING( sv1.toDense( dense ) );

            TS_ASSERT_SAME_DATA( M1, dense, sizeof(M1) );
        }

        {
            double M1[] = {
                0, 0, 0, 0,
                0, 0, 0, 0,
                5, 8, 0, 0,
                1, 2, 3, 4,
                0, 6, 0, 0,
            };

            SparseUnsortedLists sv1;
            sv1.initialize( M1, 5, 4 );

            double dense[20];

            TS_ASSERT_THROWS_NOTHING( sv1.toDense( dense ) );

            TS_ASSERT_SAME_DATA( M1, dense, sizeof(M1) );
        }
    }

    void test_get_column()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        SparseUnsortedList column( 4 );
        TS_ASSERT_THROWS_NOTHING( sv1.getColumn( 1, &column ) );
        TS_ASSERT_EQUALS( column.getNnz(), 2U );
        TS_ASSERT_EQUALS( column.get( 1 ), 8.0 );
        TS_ASSERT_EQUALS( column.get( 3 ), 6.0 );

        TS_ASSERT_THROWS_NOTHING( sv1.getColumn( 3, &column ) );
        TS_ASSERT( column.empty() );

        TS_ASSERT_THROWS_NOTHING( sv1.getColumn( 0, &column ) );
        TS_ASSERT_EQUALS( column.getNnz(), 1U );
        TS_ASSERT_EQUALS( column.get( 1 ), 5.0 );

        double dense[4];

        TS_ASSERT_THROWS_NOTHING( sv1.getColumnDense( 1, dense ) );
        TS_ASSERT_EQUALS( dense[0], 0 );
        TS_ASSERT_EQUALS( dense[1], 8 );
        TS_ASSERT_EQUALS( dense[2], 0 );
        TS_ASSERT_EQUALS( dense[3], 6 );
    }

    void test_deletions()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        double expected[] = {
            0, 0, 0, 0,
            0, 8, 0, 0,
            0, 0, 3, 0,
            0, 0, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        TS_ASSERT_THROWS_NOTHING( sv1.set( 1, 0, 0.0 ) );
        TS_ASSERT_THROWS_NOTHING( sv1.set( 3, 1, 0.0 ) );

        // Fake elements
        TS_ASSERT_THROWS_NOTHING( sv1.set( 1, 3, 0.0 ) );
        TS_ASSERT_THROWS_NOTHING( sv1.set( 3, 2, 0.0 ) );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*4 + j] );

        TS_ASSERT_THROWS_NOTHING( sv1.set( 1, 1, 0.0 ) );
        TS_ASSERT_THROWS_NOTHING( sv1.set( 2, 2, 0.0 ) );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), 0.0 );
    }

    void test_changes()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        double expected[] = {
            0, 0, 2, 0,
            5, 8, 0, 0,
            0, 0, 4, 0,
            0, 6, 0, 3,
        };

        double expected2[] = {
            0, 0, 5, 0,
            5, 8, 0, 0,
            1.5, 0, 4, 0,
            0, 6, 0, 3,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        sv1.set( 0, 2, 2.0 );
        sv1.set( 2, 2, 4.0 );
        sv1.set( 3, 3, 3.0 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*4 + j] );

        sv1.set( 0, 2, 5.0 );
        sv1.set( 2, 0, 1.5 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected2[i*4 + j] );
    }

    void test_changes_and_deletions()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        double expected[] = {
            0, 0, 2, 0,
            5, 0, 0, 0,
            0, 0, 4, 0,
            0, 0, 0, 3,
        };

        double expected2[] = {
            0, 0, 2, 0,
            5, 4, 0, 0,
            0, 0, 4, 0,
            0, 0, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        sv1.set( 0, 2, 2.0 );
        sv1.set( 2, 2, 4.0 );
        sv1.set( 3, 3, 3.0 );
        sv1.set( 1, 0, 5.0 );

        sv1.set( 1, 1, 0.0 );
        sv1.set( 3, 1, 0.0 );
        sv1.set( 3, 2, 0.0 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected[i*4 + j] );

        sv1.set( 1, 0, 5.0 );
        sv1.set( 1, 1, 4.0 );

        sv1.set( 3, 3, 0.0 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), expected2[i*4 + j] );
    }

    void test_count_elements()
    {
        double M1[] = {
            0, 0, 0, 0, 1,
            5, 8, 0, 0, 2,
            0, 2, 3, 0, 3,
            0, 0, 4, 0, 4,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 5 );

        unsigned rowElements[4];
        unsigned columnElements[5];

        TS_ASSERT_THROWS_NOTHING( sv1.countElements( rowElements, columnElements ) );

        TS_ASSERT_EQUALS( rowElements[0], 1U );
        TS_ASSERT_EQUALS( rowElements[1], 3U );
        TS_ASSERT_EQUALS( rowElements[2], 3U );
        TS_ASSERT_EQUALS( rowElements[3], 2U );

        TS_ASSERT_EQUALS( columnElements[0], 1U );
        TS_ASSERT_EQUALS( columnElements[1], 2U );
        TS_ASSERT_EQUALS( columnElements[2], 2U );
        TS_ASSERT_EQUALS( columnElements[3], 0U );
        TS_ASSERT_EQUALS( columnElements[4], 4U );
    }

    void test_transpose()
    {
        double M1[] = {
            0, 0, 0, 0, 1,
            5, 8, 0, 0, 2,
            0, 2, 3, 0, 3,
            0, 0, 4, 0, 4,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 5 );

        SparseUnsortedLists sv2;
        TS_ASSERT_THROWS_NOTHING( sv1.transposeIntoOther( &sv2 ) );

        double expected[] = {
            0, 5, 0, 0,
            0, 8, 2, 0,
            0, 0, 3, 4,
            0, 0, 0, 0,
            1, 2, 3, 4,
        };

        for ( unsigned i = 0; i < 5; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv2.get( i, j ), expected[i*4 + j] );

        SparseUnsortedLists sv3;
        TS_ASSERT_THROWS_NOTHING( sv2.transposeIntoOther( &sv3 ) );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 5; ++j )
                TS_ASSERT_EQUALS( sv3.get( i, j ), M1[i*5 + j] );

        // Transpose an empty matrix
        double empty[] = {
            0, 0, 0,
            0, 0, 0,
        };

        SparseUnsortedLists sv4;
        sv4.initialize( empty, 2, 3 );

        SparseUnsortedLists sv5;
        TS_ASSERT_THROWS_NOTHING( sv4.transposeIntoOther( &sv5 ) );

        for ( unsigned i = 0; i < 3; ++i )
            for ( unsigned j = 0; j < 2; ++j )
                TS_ASSERT_EQUALS( sv5.get( i, j ), 0.0 );
    }

    void test_clear()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        SparseUnsortedLists sv1;
        sv1.initialize( M1, 4, 4 );

        TS_ASSERT_THROWS_NOTHING( sv1.clear() );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( sv1.get( i, j ), 0.0 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
