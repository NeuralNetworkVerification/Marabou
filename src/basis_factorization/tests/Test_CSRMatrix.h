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

        for ( unsigned i = 0; i < 5; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( M2[i*4 + j], csr2.get( i, j ) );
    }

    void test_store_restore()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        CSRMatrix csr2;
        csr1.storeIntoOther( &csr2 );

        CSRMatrix csr3;
        csr2.storeIntoOther( &csr3 );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( csr1.get( i, j ), csr3.get( i, j ) );
    }

    void test_add_last_row()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        double row5[] = { 1, 2, 0, 0 };
        double row6[] = { 0, 2, -3, 0 };
        double row7[] = { 1, 0, 0, 4 };

        csr1.addLastRow( row5 );
        csr1.addLastRow( row6 );
        csr1.addLastRow( row7 );

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
                TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*4 + j] );
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

            CSRMatrix csr1;
            csr1.initialize( M1, 4, 4 );

            double col5[] = { 1, 2, 0, 0 };
            double col6[] = { 0, 2, -3, 0 };
            double col7[] = { 1, 0, 0, 4 };

            csr1.addLastColumn( col5 );
            csr1.addLastColumn( col6 );
            csr1.addLastColumn( col7 );

            double expected[] = {
                0, 0, 0, 2, 1,  0, 1,
                5, 8, 0, 0, 2,  2, 0,
                0, 0, 3, 0, 0, -3, 0,
                0, 6, 0, 0, 0,  0, 4,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 7; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*7 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 11U );
        }

        {
            double M1[] = {
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };

            CSRMatrix csr1;
            csr1.initialize( M1, 4, 4 );

            double col5[] = { 0, 0, 0, 0 };
            csr1.addLastColumn( col5 );

            double expected[] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 5; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*5 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 0U );
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

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        SparseVector row;
        TS_ASSERT_THROWS_NOTHING( csr1.getRow( 1, &row ) );
        TS_ASSERT_EQUALS( row.size(), 2U );
        TS_ASSERT_EQUALS( row._values[0], 5.0 );
        TS_ASSERT_EQUALS( row._values[1], 8.0 );

        TS_ASSERT_THROWS_NOTHING( csr1.getRow( 3, &row ) );
        TS_ASSERT_EQUALS( row.size(), 1U );
        TS_ASSERT_EQUALS( row._values[1], 6.0 );

        TS_ASSERT_THROWS_NOTHING( csr1.getRow( 0, &row ) );
        TS_ASSERT( row.empty() );
    }

    void test_to_dense()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        double dense[16];

        TS_ASSERT_THROWS_NOTHING( csr1.toDense( dense ) );

        TS_ASSERT_SAME_DATA( M1, dense, sizeof(M1) );
    }

    void test_get_column()
    {
        double M1[] = {
            0, 0, 0, 0,
            5, 8, 0, 0,
            0, 0, 3, 0,
            0, 6, 0, 0,
        };

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        SparseVector column;
        TS_ASSERT_THROWS_NOTHING( csr1.getColumn( 1, &column ) );
        TS_ASSERT_EQUALS( column.size(), 2U );
        TS_ASSERT_EQUALS( column._values[1], 8.0 );
        TS_ASSERT_EQUALS( column._values[3], 6.0 );

        TS_ASSERT_THROWS_NOTHING( csr1.getColumn( 3, &column ) );
        TS_ASSERT( column.empty() );

        TS_ASSERT_THROWS_NOTHING( csr1.getColumn( 0, &column ) );
        TS_ASSERT_EQUALS( column.size(), 1U );
        TS_ASSERT_EQUALS( column._values[1], 5.0 );

        double dense[4];

        TS_ASSERT_THROWS_NOTHING( csr1.getColumnDense( 1, dense ) );
        TS_ASSERT_EQUALS( dense[0], 0 );
        TS_ASSERT_EQUALS( dense[1], 8 );
        TS_ASSERT_EQUALS( dense[2], 0 );
        TS_ASSERT_EQUALS( dense[3], 6 );
    }

    void test_merge_columns()
    {
        {
            double M1[] = {
                0, 0, 0, 0,
                5, 8, 0, 0,
                0, 2, 3, 0,
                0, 0, 4, 0,
            };

            CSRMatrix csr1;
            csr1.initialize( M1, 4, 4 );

            TS_ASSERT_THROWS_NOTHING( csr1.mergeColumns( 1, 2 ) );

            double expected[] = {
                0, 0, 0,
                5, 8, 0,
                0, 5, 0,
                0, 4, 0,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 3; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*3 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 4U );
        }

        {
            double M1[] = {
                0, 0, -1, 1,
                5, 8, 0, 1,
                0, 2, 3, 0,
                0, 0, 4, 1,
            };

            CSRMatrix csr1;
            csr1.initialize( M1, 4, 4 );

            TS_ASSERT_THROWS_NOTHING( csr1.mergeColumns( 2, 3 ) );

            double expected[] = {
                0, 0, 0,
                5, 8, 1,
                0, 2, 3,
                0, 0, 5,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 3; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*3 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 6U );

            double newRow[] = { 1, 2, 3 };
            TS_ASSERT_THROWS_NOTHING( csr1.addLastRow( newRow ) );

            double expected2[] = {
                0, 0, 0,
                5, 8, 1,
                0, 2, 3,
                0, 0, 5,
                1, 2, 3,
            };

            for ( unsigned i = 0; i < 5; ++i )
                for ( unsigned j = 0; j < 3; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected2[i*3 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 9U );
        }

        {
            double M1[] = {
                0, 0, -1, 1,
                0, 0, -1, 1,
                0, 0,  0, 0,
                0, 0, -1, 1,
            };

            CSRMatrix csr1;
            csr1.initialize( M1, 4, 4 );

            TS_ASSERT_THROWS_NOTHING( csr1.mergeColumns( 2, 3 ) );

            double expected[] = {
                0, 0, 0,
                0, 0, 0,
                0, 0, 0,
                0, 0, 0,
            };

            for ( unsigned i = 0; i < 4; ++i )
                for ( unsigned j = 0; j < 3; ++j )
                    TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*3 + j] );

            TS_ASSERT_EQUALS( csr1.getNnz(), 0U );
        }
    }

    void test_erasures()
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

        CSRMatrix csr1;
        csr1.initialize( M1, 4, 4 );

        csr1.commitChange( 1, 0, 0.0 );
        csr1.commitChange( 3, 1, 0.0 );

        // No changes before "execute" is called
        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( csr1.get( i, j ), M1[i*4 + j] );

        TS_ASSERT_THROWS_NOTHING( csr1.executeChanges() );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*4 + j] );

        // Fake elements
        csr1.commitChange( 1, 3, 0.0 );
        csr1.commitChange( 3, 2, 0.0 );
        TS_ASSERT_THROWS_NOTHING( csr1.executeChanges() );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( csr1.get( i, j ), expected[i*4 + j] );

        csr1.commitChange( 1, 1, 0.0 );
        csr1.commitChange( 2, 2, 0.0 );
        TS_ASSERT_THROWS_NOTHING( csr1.executeChanges() );

        for ( unsigned i = 0; i < 4; ++i )
            for ( unsigned j = 0; j < 4; ++j )
                TS_ASSERT_EQUALS( csr1.get( i, j ), 0.0 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
