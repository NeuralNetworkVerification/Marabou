/*********************                                                        */
/*! \file Test_RoundConstraint.h
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

#include "MarabouError.h"
#include "MockTableau.h"
#include "Query.h"
#include "RoundConstraint.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MockForRoundConstraint
{
public:
};

class MaxConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForRoundConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForRoundConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_round_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        RoundConstraint round( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = round.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( round.participatingVariable( b ) );
        TS_ASSERT( round.participatingVariable( f ) );
        TS_ASSERT( !round.participatingVariable( 0 ) );
        TS_ASSERT( !round.participatingVariable( 2 ) );
        TS_ASSERT( !round.participatingVariable( 3 ) );
        TS_ASSERT( !round.participatingVariable( 5 ) );

        // not obsolete yet
        TS_ASSERT( !round.constraintObsolete() );

        // eliminate variable b
        round.eliminateVariable( b, 0 ); // 0 is dummy for the argument of fixedValue

        // round is obsolete now
        TS_ASSERT( round.constraintObsolete() );
    }

    void test_round_duplicate()
    {
        unsigned b = 1;
        unsigned f = 4;

        RoundConstraint *round = new RoundConstraint( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = round->getParticipatingVariables() );

        // not obsolete yet
        TS_ASSERT( !round->constraintObsolete() );

        // duplicate constraint
        RoundConstraint *round2 = dynamic_cast<RoundConstraint *>( round->duplicateConstraint() );
        TS_ASSERT_THROWS_NOTHING( participatingVariables = round2->getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );
        TS_ASSERT( round2->participatingVariable( b ) );
        TS_ASSERT( round2->participatingVariable( f ) );
        TS_ASSERT( !round2->participatingVariable( 0 ) );
        TS_ASSERT( !round2->participatingVariable( 2 ) );
        TS_ASSERT( !round2->participatingVariable( 3 ) );
        TS_ASSERT( !round2->participatingVariable( 5 ) );

        // eliminate variable b
        round->eliminateVariable( b, 0 ); // 0 is dummy for the argument of fixedValue

        // round is obsolete now
        TS_ASSERT( round->constraintObsolete() );

        // round2 is not obsolete
        TS_ASSERT( !round2->constraintObsolete() );

        TS_ASSERT_THROWS_NOTHING( delete round );
        TS_ASSERT_THROWS_NOTHING( delete round2 );
    }

    bool existsBound( const List<Tightening> &bounds, const Tightening &t )
    {
        for ( const auto &bound : bounds )
        {
            if ( bound._type == t._type && bound._variable == t._variable &&
                 FloatUtils::areEqual( bound._value, t._value, 0.000001 ) )
                return true;
        }
        std::cout << "Bound does not exists: ";
        t.dump();
        return false;
    }

    bool testBound( unsigned b,
                    unsigned f,
                    unsigned variable,
                    double lb,
                    double ub,
                    const List<Tightening> &expectedBounds )
    {
        RoundConstraint round( b, f );

        round.notifyLowerBound( variable, lb );
        round.notifyUpperBound( variable, ub );

        List<Tightening> tightenings;
        round.getEntailedTightenings( tightenings );

        bool exists = true;
        for ( const auto &bound : tightenings )
        {
            exists = exists && existsBound( expectedBounds, bound );
        }
        return exists;
    }

    void test_round_notify_bounds_b()
    {
        unsigned b = 1;
        unsigned f = 4;

        {
            // Case 1
            List<Tightening> expectedTightenings = { Tightening( b, -10, Tightening::LB ),
                                                     Tightening( b, -10, Tightening::UB ),
                                                     Tightening( f, -10, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, -10, -10, expectedTightenings ) );
        }
        {
            // Case 2
            List<Tightening> expectedTightenings = { Tightening( b, -9.9, Tightening::LB ),
                                                     Tightening( b, -9.9, Tightening::UB ),
                                                     Tightening( f, -10, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, -9.9, -9.9, expectedTightenings ) );
        }
        {
            // Case 3
            List<Tightening> expectedTightenings = { Tightening( b, -9.5, Tightening::LB ),
                                                     Tightening( b, -9.5, Tightening::UB ),
                                                     Tightening( f, -10, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, -9.5, -9.5, expectedTightenings ) );
        }
        {
            // Case 4
            List<Tightening> expectedTightenings = { Tightening( b, -9.3, Tightening::LB ),
                                                     Tightening( b, -9.3, Tightening::UB ),
                                                     Tightening( f, -9, Tightening::LB ),
                                                     Tightening( f, -9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, -9.3, -9.3, expectedTightenings ) );
        }
        {
            // Case 5
            List<Tightening> expectedTightenings = { Tightening( b, -8.5, Tightening::LB ),
                                                     Tightening( b, -8.5, Tightening::UB ),
                                                     Tightening( f, -8, Tightening::LB ),
                                                     Tightening( f, -8, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, -8.5, -8.5, expectedTightenings ) );
        }
        {
            // Case 6
            List<Tightening> expectedTightenings = { Tightening( b, 10, Tightening::LB ),
                                                     Tightening( b, 10, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, 10, 10, expectedTightenings ) );
        }
        {
            // Case 7
            List<Tightening> expectedTightenings = { Tightening( b, 9.9, Tightening::LB ),
                                                     Tightening( b, 9.9, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, 9.9, 9.9, expectedTightenings ) );
        }
        {
            // Case 8
            List<Tightening> expectedTightenings = { Tightening( b, 9.5, Tightening::LB ),
                                                     Tightening( b, 9.5, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, 9.5, 9.5, expectedTightenings ) );
        }
        {
            // Case 9
            List<Tightening> expectedTightenings = { Tightening( b, 9.3, Tightening::LB ),
                                                     Tightening( b, 9.3, Tightening::UB ),
                                                     Tightening( f, 9, Tightening::LB ),
                                                     Tightening( f, 9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, 9.3, 9.3, expectedTightenings ) );
        }
        {
            // Case 10
            List<Tightening> expectedTightenings = { Tightening( b, 8.5, Tightening::LB ),
                                                     Tightening( b, 8.5, Tightening::UB ),
                                                     Tightening( f, 8, Tightening::LB ),
                                                     Tightening( f, 8, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, b, 8.5, 8.5, expectedTightenings ) );
        }
    }

    void test_round_notify_bounds_f()
    {
        unsigned b = 1;
        unsigned f = 4;
        {
            // Case 1
            List<Tightening> expectedTightenings = { Tightening( b, -10.5, Tightening::LB ),
                                                     Tightening( b, -9.5, Tightening::UB ),
                                                     Tightening( f, -10, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, -10, -10, expectedTightenings ) );
        }
        {
            // Case 2
            List<Tightening> expectedTightenings = { Tightening( b, -9.5, Tightening::LB ),
                                                     Tightening( b, -9.5, Tightening::UB ),
                                                     Tightening( f, -9, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, -9.9, -9.9, expectedTightenings ) );
        }
        {
            // Case 3
            List<Tightening> expectedTightenings = { Tightening( b, -9.5, Tightening::LB ),
                                                     Tightening( b, -9.5, Tightening::UB ),
                                                     Tightening( f, -9, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, -9.5, -9.5, expectedTightenings ) );
        }
        {
            // Case 4
            List<Tightening> expectedTightenings = { Tightening( b, -9.5, Tightening::LB ),
                                                     Tightening( b, -9.5, Tightening::UB ),
                                                     Tightening( f, -9, Tightening::LB ),
                                                     Tightening( f, -10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, -9.3, -9.3, expectedTightenings ) );
        }
        {
            // Case 5
            List<Tightening> expectedTightenings = { Tightening( b, -8.5, Tightening::LB ),
                                                     Tightening( b, -8.5, Tightening::UB ),
                                                     Tightening( f, -8, Tightening::LB ),
                                                     Tightening( f, -9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, -8.5, -8.5, expectedTightenings ) );
        }
        {
            // Case 6
            List<Tightening> expectedTightenings = { Tightening( b, 9.5, Tightening::LB ),
                                                     Tightening( b, 10.5, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 10, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, 10, 10, expectedTightenings ) );
        }
        {
            // Case 7
            List<Tightening> expectedTightenings = { Tightening( b, 9.5, Tightening::LB ),
                                                     Tightening( b, 9.5, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, 9.9, 9.9, expectedTightenings ) );
        }
        {
            // Case 8
            List<Tightening> expectedTightenings = { Tightening( b, 9.5, Tightening::LB ),
                                                     Tightening( b, 9.5, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, 9.5, 9.5, expectedTightenings ) );
        }
        {
            // Case 9
            List<Tightening> expectedTightenings = { Tightening( b, 9.5, Tightening::LB ),
                                                     Tightening( b, 9.5, Tightening::UB ),
                                                     Tightening( f, 10, Tightening::LB ),
                                                     Tightening( f, 9, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, 9.3, 9.3, expectedTightenings ) );
        }
        {
            // Case 10
            List<Tightening> expectedTightenings = { Tightening( b, 8.5, Tightening::LB ),
                                                     Tightening( b, 8.5, Tightening::UB ),
                                                     Tightening( f, 9, Tightening::LB ),
                                                     Tightening( f, 8, Tightening::UB ) };

            TS_ASSERT( testBound( b, f, f, 8.5, 8.5, expectedTightenings ) );
        }
    }

    void test_round_update_variable_index()
    {
        unsigned b = 0;
        unsigned f = 1;
        unsigned new_b = 2;
        unsigned new_f = 3;

        RoundConstraint round( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = round.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( round.participatingVariable( b ) );
        TS_ASSERT( round.participatingVariable( f ) );
        TS_ASSERT( !round.participatingVariable( new_b ) );
        TS_ASSERT( !round.participatingVariable( new_f ) );

        round.notifyLowerBound( b, -1.0 );
        round.notifyUpperBound( b, 1.0 );


        // update variable index
        round.updateVariableIndex( b, new_b );
        round.updateVariableIndex( f, new_f );

        TS_ASSERT( !round.participatingVariable( b ) );
        TS_ASSERT( !round.participatingVariable( f ) );
        TS_ASSERT_EQUALS( round.getB(), new_b );
        TS_ASSERT_EQUALS( round.getF(), new_f );
        TS_ASSERT( round.participatingVariable( new_b ) );
        TS_ASSERT( round.participatingVariable( new_f ) );
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        RoundConstraint round( b, f );

        TS_ASSERT_THROWS_NOTHING( round.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &round ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &round ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( round.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &round ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &round ) );
    }

    void test_round_serialize()
    {
        unsigned b = 0;
        unsigned f = 1;

        RoundConstraint round( b, f );

        String serializedString = round.serializeToString();

        RoundConstraint serializedRound( serializedString.ascii() );

        TS_ASSERT_EQUALS( serializedRound.getB(), b );
        TS_ASSERT_EQUALS( serializedRound.getF(), f );
    }

    void test_round_restore()
    {
        unsigned b_1 = 0;
        unsigned f_1 = 1;
        unsigned b_2 = 2;
        unsigned f_2 = 3;

        RoundConstraint round1( b_1, f_1 );
        TS_ASSERT_EQUALS( round1.getB(), b_1 );
        TS_ASSERT_EQUALS( round1.getF(), f_1 );

        RoundConstraint round2( b_2, f_2 );
        TS_ASSERT_EQUALS( round2.getB(), b_2 );
        TS_ASSERT_EQUALS( round2.getF(), f_2 );

        // restore
        round1.restoreState( &round2 );
        TS_ASSERT_EQUALS( round1.getB(), b_2 );
        TS_ASSERT_EQUALS( round1.getF(), f_2 );
    }

    void test_round_satisfied()
    {
        unsigned b1 = 0;
        unsigned f1 = 1;

        RoundConstraint round( b1, f1 );
        MockTableau tableau;
        round.registerTableau( &tableau );
        tableau.setValue( b1, 1.4 );
        tableau.setValue( f1, 2 );
        TS_ASSERT( !round.satisfied() );

        tableau.setValue( b1, 1.5 );
        tableau.setValue( f1, 2 );
        TS_ASSERT( round.satisfied() );

        tableau.setValue( b1, 1.3 );
        tableau.setValue( f1, 1 );
        TS_ASSERT( round.satisfied() );

        tableau.setValue( b1, 2.8 );
        tableau.setValue( f1, 2 );
        TS_ASSERT( !round.satisfied() );

        tableau.setValue( b1, 2 );
        tableau.setValue( f1, 2 );
        TS_ASSERT( round.satisfied() );
    }
};
