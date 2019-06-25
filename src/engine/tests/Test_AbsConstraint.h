//
// Created by shirana on 6/13/19.
//

#ifndef MARABOU_TEST_ABSCONSTRAINT_H
#define MARABOU_TEST_ABSCONSTRAINT_H



#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "AbsConstraint.h"
#include "AbsError.h"

#include <string.h>


class MockForAbsConstraint
        : public MockErrno
{
public:
};

class AbsConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForAbsConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForAbsConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_abs_entailed_tighteningst() {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        abs.notifyUpperBound( b, 7 );
        abs.notifyUpperBound( f, 7 );

        abs.notifyLowerBound( b, -1 );
        abs.notifyLowerBound( f, 0 );
        abs.getEntailedTightenings( entailedTightenings );
//        TS_ASSERT( entailedTightenings.empty() );

        TS_ASSERT(abs.duplicateConstraint())
    }
};
#endif //MARABOU_TEST_ABSCONSTRAINT_H
