/*********************                                                        */
/*! \file Test_BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "BoundManager.h"
#include "context/cdlist.h"
#include "context/context.h"
#include "FloatUtils.h"

class BoundManagerTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_bound_manager_initialize()
    {
        CVC4::context::Context context;
        BoundManager boundManager(context);

        unsigned numberOfVariables = 5u;
        TS_ASSERT_THROWS_NOTHING( boundManager.initialize( numberOfVariables) );

        for ( unsigned i = 0; i < numberOfVariables; ++i)
        {
          TS_ASSERT( boundManager.getLowerBound( i ) >= FloatUtils::negativeInfinity() ) ;
          TS_ASSERT( boundManager.getUpperBound( i ) <= FloatUtils::infinity() ) ;
        }


        /* RowBoundTightener tightener( *tableau, boundManager ); */

        /* tableau->setDimensions( 2, 5 ); */

        /* // Current bounds: */
        /* //  0 <= x0 <= 0 */
        /* //    5  <= x1 <= 10 */
        /* //    -2 <= x2 <= 3 */
        /* //  -100 <= x4 <= 100 */
        /* tableau->setLowerBound( 0, -200 ); */
        /* tableau->setUpperBound( 0, 200 ); */
        /* tableau->setLowerBound( 1, 5 ); */
        /* tableau->setUpperBound( 1, 10 ); */
        /* tableau->setLowerBound( 2, -2 ); */
        /* tableau->setUpperBound( 2, 3 ); */
        /* tableau->setLowerBound( 3, -5 ); */
        /* tableau->setUpperBound( 3, 5 ); */
        /* tableau->setLowerBound( 4, -100 ); */
        /* tableau->setUpperBound( 4, 100 ); */

        /* tightener.setDimensions(); */

        /* TableauRow row( 3 ); */
        /* // 1 - x0 - x1 + 2x2 */
        /* row._row[0] = TableauRow::Entry( 0, -1 ); */
        /* row._row[1] = TableauRow::Entry( 1, -1 ); */
        /* row._row[2] = TableauRow::Entry( 2, 2 ); */
        /* row._scalar = 1; */
        /* row._lhs = 4; */

        /* tableau->nextPivotRow = &row; */

        /* // 1 - x0 - x1 + 2x2 = x4 (pre pivot) */
        /* // x0 entering, x4 leaving */
        /* // x0 = 1 - x1 + 2 x2 - x4 */

        /* TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() ); */

        /* List<Tightening> tightenings; */
        /* TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) ); */

        /* // Lower and upper bounds should have been tightened */
        /* TS_ASSERT_EQUALS( tightenings.size(), 2U ); */

        /* auto lower = tightenings.begin(); */
        /* while ( ( lower != tightenings.end() ) && !( ( lower->_variable == 0 ) && ( lower->_type == Tightening::LB ) ) ) */
        /*     ++lower; */
        /* TS_ASSERT( lower != tightenings.end() ); */

        /* auto upper = tightenings.begin(); */
        /* while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 0 ) && ( upper->_type == Tightening::UB ) ) ) */
        /*     ++upper; */
        /* TS_ASSERT( upper != tightenings.end() ); */

        /* // LB -> 1 - 10 - 4 -100 */
        /* // UB -> 1 - 5 + 6 + 100 */
        /* TS_ASSERT_EQUALS( lower->_value, -113 ); */
        /* TS_ASSERT_EQUALS( upper->_value, 102 ); */
    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
