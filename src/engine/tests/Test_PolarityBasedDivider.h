/*********************                                                        */
/*! \file Test_PolarityDivider.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "MockEngine.h"
#include "InputQuery.h"
#include "PiecewiseLinearConstraint.h"
#include "PolarityBasedDivider.h"
#include "List.h"
#include "MStringf.h"
#include "ReluConstraint.h"
#include "SubQuery.h"
#include "Vector.h"

class PolarityBasedDividerTestSuite : public CxxTest::TestSuite
{
public:

    std::shared_ptr<MockEngine> engine;
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void add_to_split( PiecewiseLinearCaseSplit &split, PiecewiseLinearCaseSplit &toAdd )
    {
        for ( const auto &bound : toAdd.getBoundTightenings() )
            split.storeBoundTightening( bound );
    }

    void test_create_subqueries()
    {

        TS_ASSERT_THROWS_NOTHING( engine = std::make_shared<MockEngine>() );

        PiecewiseLinearConstraint *relu1 = new ReluConstraint( 1, 2 );

        PiecewiseLinearConstraint *relu2 = new ReluConstraint( 3, 4 );

        engine->setSplitPLConstraint( relu1 );
        engine->setSplitPLConstraint( relu2 );
        engine->setSplitPLConstraint( relu2 );

        // Initiate the divider
        auto queryDivider = std::unique_ptr<PolarityBasedDivider>
            ( new PolarityBasedDivider( engine ) );
        double timeoutFactor = 1.5;

        // Use the divider to create 4 subqueries

        unsigned numNewSubQueries = 4;
        String queryId = "mock";
        unsigned timeoutInSeconds = 5;
        auto previousSplit = std::unique_ptr<PiecewiseLinearCaseSplit>
            ( new PiecewiseLinearCaseSplit );

        // Divide the previousSplit
        SubQueries subQueries;
        queryDivider->createSubQueries( numNewSubQueries, queryId,
                                        *previousSplit, (unsigned)
                                        timeoutInSeconds * timeoutFactor,
                                        subQueries );
        TS_ASSERT_EQUALS( subQueries.size(), numNewSubQueries );

        List<PiecewiseLinearCaseSplit> splits1 = relu1->getCaseSplits();
        List<PiecewiseLinearCaseSplit> splits2 = relu2->getCaseSplits();

        TS_ASSERT_EQUALS( splits1.size(), 2u );
        TS_ASSERT_EQUALS( splits2.size(), 2u );

        auto split1_1 = *splits1.begin();
        auto split1_2 = *splits1.rbegin();
        auto split2_1 = *splits2.begin();
        auto split2_2 = *splits2.rbegin();

        PiecewiseLinearCaseSplit split_act_act;
        add_to_split( split_act_act, split1_1);
        add_to_split( split_act_act, split2_1);

        PiecewiseLinearCaseSplit split_act_inact;
        add_to_split( split_act_inact, split1_1);
        add_to_split( split_act_inact, split2_2);

        PiecewiseLinearCaseSplit split_inact_act;
        add_to_split( split_inact_act, split1_2);
        add_to_split( split_inact_act, split2_1);

        PiecewiseLinearCaseSplit split_inact_inact;
        add_to_split( split_inact_inact, split1_2);
        add_to_split( split_inact_inact, split2_2);

        // The following four splits should be created by the queryDivider
        Vector<PiecewiseLinearCaseSplit> newSplits;
        newSplits.append( split_act_act );
        newSplits.append( split_act_inact );
        newSplits.append( split_inact_act );
        newSplits.append( split_inact_inact );

        unsigned correctTimeoutInSeconds = (unsigned) ( timeoutInSeconds *
                                                        timeoutFactor );
        unsigned index = 0;
        for ( const auto &subQuery : subQueries )
        {
            TS_ASSERT( subQuery->_queryId == queryId +
                       Stringf( "-%u", index + 1 ) );
            TS_ASSERT( *(subQuery->_split) == newSplits[index] );
            TS_ASSERT( subQuery->_timeoutInSeconds == correctTimeoutInSeconds );
            ++index;

            delete subQuery;
        }

        delete relu1;
        delete relu2;
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
