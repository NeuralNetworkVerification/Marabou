/*********************                                                        */
/*! \file Test_Equation.h
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

#include "Equation.h"

#include <cxxtest/TestSuite.h>

class EquationTestSuite : public CxxTest::TestSuite
{
public:
    void test_remove_redundant_addends()
    {
        Equation eq;
        eq.addAddend( 1.5, 1 );
        eq.addAddend( 1.5, 1 );
        eq.addAddend( -1, 2 );
        eq.addAddend( 1, 2 );
        eq.addAddend( -0.5, 1 );
        eq.addAddend( 0, 1 );
        eq.addAddend( 0, 4 );
        eq.addAddend( 3.5, 5 );
        eq.setScalar( 1 );

        TS_ASSERT_EQUALS( eq._addends.size(), 8u );
        TS_ASSERT( eq.containsRedundantAddends() );

        // After transformation, should become:
        // 2.5 x1 + 3.5 x5 = 1
        TS_ASSERT_THROWS_NOTHING( eq.removeRedundantAddends() );
        eq.dump();
        TS_ASSERT_EQUALS( eq._addends.size(), 2u );
        Set<unsigned> participatingVariables = eq.getParticipatingVariables();
        TS_ASSERT_EQUALS( participatingVariables.size(), 2u );
        TS_ASSERT( participatingVariables.exists( 1 ) );
        TS_ASSERT( participatingVariables.exists( 5 ) );
        TS_ASSERT_EQUALS( eq.getCoefficient( 1 ), 2.5 );
        TS_ASSERT_EQUALS( eq.getCoefficient( 5 ), 3.5 );
        TS_ASSERT_EQUALS( eq.getCoefficient( 0 ), 0 );
        TS_ASSERT( !eq.containsRedundantAddends() );
    }
};
