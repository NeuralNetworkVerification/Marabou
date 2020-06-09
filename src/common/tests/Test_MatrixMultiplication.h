/*********************                                                        */
/*! \file Test_FloatUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
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

#include "MatrixMultiplication.h"


class MatrixMultiplicationTestSuite : public CxxTest::TestSuite
{
public:

    void test_vector_matrix()
    {
        double matA[] = {1,2};
        double matB[] = {1,2,3,1};
        double matC[2] = {0};
        unsigned rowsA = 1;
        unsigned columnsA = 2;
        unsigned columnsB = 2;
        matrixMultiplication(matA, matB, matC, rowsA, columnsA, columnsB);

        // matC[0] = 1*1+2*3 = 7
        // matC[1] = 1*2+2*1 = 4
        TS_ASSERT(matC[0] == 7);
        TS_ASSERT(matC[1] == 4);
    }

    void test_matrix_matrix()
    {
        double matA[] = {1,2,3,4,5,6}; // [1,2], [3,4], [5,6]
        double matB[] = {1,2,3,4}; // [1,2], [3,4]
        double matC[6] = {0};
        unsigned rowsA = 3;
        unsigned columnsA = 2;
        unsigned columnsB = 2;
        matrixMultiplication(matA, matB, matC, rowsA, columnsA, columnsB);

        TS_ASSERT(matC[0] == 7);
        TS_ASSERT(matC[1] == 10);
        TS_ASSERT(matC[2] == 15);
        TS_ASSERT(matC[3] == 22);
        TS_ASSERT(matC[4] == 23);
        TS_ASSERT(matC[5] == 34);
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
