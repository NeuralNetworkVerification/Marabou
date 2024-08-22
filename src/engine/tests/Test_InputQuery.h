/*********************                                                        */
/*! \file Test_InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "InputQuery.h"
#include "MarabouError.h"
#include "ReluConstraint.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class CDQueryTestSuite : public CxxTest::TestSuite
{
public:
    void test_set_and_get_new_variable()
    {
        InputQuery inputQuery;
        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 1 ) );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 1u );
        TS_ASSERT_EQUALS( inputQuery.getNewVariable(), 1u );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 2u );
        TS_ASSERT_EQUALS( inputQuery.getNewVariable(), 2u );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 3u );
    }
};
