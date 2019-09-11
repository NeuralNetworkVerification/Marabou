/*********************                                                        */
/*! \file Test_LargestIntervalDivider.h
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

#include "LargestIntervalDivider.h"
#include "List.h"
#include "MStringf.h"
#include "SubQuery.h"
#include "Vector.h"

class LargestIntervalDividerTestSuite : public CxxTest::TestSuite
{
public:

    std::unique_ptr<LargestIntervalDivider> queryDivider;
    List<unsigned> inputVariables;
    double timeoutFactor;

    void setUp()
    {
        // inputVariables x1 x2 x3
        inputVariables.append( 1 );
        inputVariables.append( 2 );
        inputVariables.append( 3 );
        timeoutFactor = 1.5;

        // Initiate the divider
        queryDivider = std::unique_ptr<LargestIntervalDivider>
            ( new LargestIntervalDivider( inputVariables ) );
    }

    void tearDown()
    {
        inputVariables.clear();
    }

    void test_create_subqueries()
    {
        //   Input Region:
        //   -2 <= x1 <= 2
        //    3 <= x2 <= 5
        //    2 <= x3 <= 5
        //
        //    Timeout factor 1.5
        //    Previous Timeout 5
        //    Divide into 4 subqueries
        //
        // 1. The number of created regions must match the corresponding
        //    argument
        //
        // 2. The new query ids must be "<oldQueryId>-1", "<oldQueryId>-2" ...
        //
        // 3. Check the output ranges
        //
        // Step 1 ( bisect the range of x1 )
        //   -2 <= x1 <= 0
        //    3 <= x2 <= 5
        //    2 <= x3 <= 5
        //
        //    0 <= x1 <= 2
        //    3 <= x2 <= 5
        //    2 <= x3 <= 5
        //
        // Step 2 ( bisect the range of x3 )
        //   -2 <= x1 <= 0
        //    3 <= x2 <= 5
        //    2 <= x3 <= 3.5
        //
        //    -2 <= x1 <= 0
        //    3 <= x2 <= 5
        //    3.5 <= x3 <= 5
        //
        //    0 <= x1 <= 2
        //    3 <= x2 <= 5
        //    2 <= x3 <= 3.5
        //
        //    0 <= x1 <= 2
        //    3 <= x2 <= 5
        //    3.5 <= x3 <= 5
        //
        // 3. The timeout of each subquery must be (timeoutFactor *
        //    previousTimeout)

        unsigned numNewSubQueries = 4;
        String queryId = "mock";
        unsigned timeoutInSeconds = 5;

        auto previousSplit = std::unique_ptr<PiecewiseLinearCaseSplit>
            ( new PiecewiseLinearCaseSplit );
        Tightening bound1( 1, -2.0, Tightening::LB );
        Tightening bound2( 1, 2.0, Tightening::UB );
        Tightening bound3( 2, 3.0, Tightening::LB );
        Tightening bound4( 2, 5.0, Tightening::UB );
        Tightening bound5( 3, 2.0, Tightening::LB );
        Tightening bound6( 3, 5.0, Tightening::UB );

        previousSplit->storeBoundTightening( bound1 );
        previousSplit->storeBoundTightening( bound2 );
        previousSplit->storeBoundTightening( bound3 );
        previousSplit->storeBoundTightening( bound4 );
        previousSplit->storeBoundTightening( bound5 );
        previousSplit->storeBoundTightening( bound6 );

        // Divide the previousSplit
        SubQueries subQueries;
        queryDivider->createSubQueries( numNewSubQueries, queryId,
                                        *previousSplit, (unsigned)
                                        timeoutInSeconds * timeoutFactor,
                                        subQueries );

        // The following four splits should be created by the queryDivider
        Vector<PiecewiseLinearCaseSplit> newSplits;
        PiecewiseLinearCaseSplit newSplit1;
        Tightening bound1_1( 1, -2.0, Tightening::LB );
        Tightening bound2_1( 1, 0.0, Tightening::UB );
        Tightening bound3_1( 2, 3.0, Tightening::LB );
        Tightening bound4_1( 2, 5.0, Tightening::UB );
        Tightening bound5_1( 3, 2.0, Tightening::LB );
        Tightening bound6_1( 3, 3.5, Tightening::UB );

        newSplit1.storeBoundTightening( bound1_1 );
        newSplit1.storeBoundTightening( bound2_1 );
        newSplit1.storeBoundTightening( bound3_1 );
        newSplit1.storeBoundTightening( bound4_1 );
        newSplit1.storeBoundTightening( bound5_1 );
        newSplit1.storeBoundTightening( bound6_1 );

        PiecewiseLinearCaseSplit newSplit2;
        Tightening bound1_2( 1, -2.0, Tightening::LB );
        Tightening bound2_2( 1, 0.0, Tightening::UB );
        Tightening bound3_2( 2, 3.0, Tightening::LB );
        Tightening bound4_2( 2, 5.0, Tightening::UB );
        Tightening bound5_2( 3, 3.5, Tightening::LB );
        Tightening bound6_2( 3, 5.0, Tightening::UB );

        newSplit2.storeBoundTightening( bound1_2 );
        newSplit2.storeBoundTightening( bound2_2 );
        newSplit2.storeBoundTightening( bound3_2 );
        newSplit2.storeBoundTightening( bound4_2 );
        newSplit2.storeBoundTightening( bound5_2 );
        newSplit2.storeBoundTightening( bound6_2 );

        PiecewiseLinearCaseSplit newSplit3;
        Tightening bound1_3( 1, 0.0, Tightening::LB );
        Tightening bound2_3( 1, 2.0, Tightening::UB );
        Tightening bound3_3( 2, 3.0, Tightening::LB );
        Tightening bound4_3( 2, 5.0, Tightening::UB );
        Tightening bound5_3( 3, 2.0, Tightening::LB );
        Tightening bound6_3( 3, 3.5, Tightening::UB );

        newSplit3.storeBoundTightening( bound1_3 );
        newSplit3.storeBoundTightening( bound2_3 );
        newSplit3.storeBoundTightening( bound3_3 );
        newSplit3.storeBoundTightening( bound4_3 );
        newSplit3.storeBoundTightening( bound5_3 );
        newSplit3.storeBoundTightening( bound6_3 );

        PiecewiseLinearCaseSplit newSplit4;
        Tightening bound1_4( 1, 0.0, Tightening::LB );
        Tightening bound2_4( 1, 2.0, Tightening::UB );
        Tightening bound3_4( 2, 3.0, Tightening::LB );
        Tightening bound4_4( 2, 5.0, Tightening::UB );
        Tightening bound5_4( 3, 3.5, Tightening::LB );
        Tightening bound6_4( 3, 5.0, Tightening::UB );

        newSplit4.storeBoundTightening( bound1_4 );
        newSplit4.storeBoundTightening( bound2_4 );
        newSplit4.storeBoundTightening( bound3_4 );
        newSplit4.storeBoundTightening( bound4_4 );
        newSplit4.storeBoundTightening( bound5_4 );
        newSplit4.storeBoundTightening( bound6_4 );

        newSplits.append( newSplit1 );
        newSplits.append( newSplit2 );
        newSplits.append( newSplit3 );
        newSplits.append( newSplit4 );

        TS_ASSERT( subQueries.size() == 4 );
        unsigned correctTimeoutInSeconds = (unsigned) ( timeoutInSeconds *
                                                        timeoutFactor );
        unsigned index = 0;
        for ( const auto &subQuery : subQueries )
            {
                TS_ASSERT( subQuery->_queryId == queryId +
                           Stringf( "-%u", index + 1 ) )
                    TS_ASSERT( *(subQuery->_split) == newSplits[index] );
                TS_ASSERT( subQuery->_timeoutInSeconds == correctTimeoutInSeconds );
                index++;
                delete subQuery;

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
