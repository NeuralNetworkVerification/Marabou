/*********************                                                        */
/*! \file Test_QuadraticConstraint.h
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

#include "FloatUtils.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include "QuadraticConstraint.h"
#include "Tightening.h"
#include <string.h>

class QuadraticConstraintTestSuite : public CxxTest::TestSuite
{
public:

  void setUp()
  {
  }

  void tearDown()
  {
  }

  void test_tighten_bounds1()
  {
    QuadraticConstraint softmax (0, 1, 2);

    softmax.notifyLowerBound(0, -2);
    softmax.notifyUpperBound(0, 1);
    softmax.notifyLowerBound(1, -1);
    softmax.notifyUpperBound(1, 2);

    List<Tightening> t;
    TS_ASSERT_THROWS_NOTHING(softmax.getEntailedTightenings(t));
    TS_ASSERT_EQUALS( t.size(), 2u );

    TS_ASSERT( containsTightening(t, Tightening(2, -4, Tightening::LB)) );
    TS_ASSERT( containsTightening(t, Tightening(2, 2, Tightening::UB)) );
  }

  void test_tighten_bounds2()
  {
    QuadraticConstraint softmax (0, 1, 2);

    softmax.notifyLowerBound(0, 1);
    softmax.notifyUpperBound(0, 2);
    softmax.notifyLowerBound(1, -1);
    softmax.notifyUpperBound(1, 2);

    List<Tightening> t;
    TS_ASSERT_THROWS_NOTHING(softmax.getEntailedTightenings(t));
    TS_ASSERT_EQUALS( t.size(), 2u );

    TS_ASSERT( containsTightening(t, Tightening(2, -2, Tightening::LB)) );
    TS_ASSERT( containsTightening(t, Tightening(2, 4, Tightening::UB)) );
  }

  void test_tighten_bounds3()
  {
    QuadraticConstraint softmax (0, 1, 2);

    softmax.notifyLowerBound(0, -2);
    softmax.notifyUpperBound(0, -1);
    softmax.notifyLowerBound(1, -3);
    softmax.notifyUpperBound(1, -2);

    List<Tightening> t;
    TS_ASSERT_THROWS_NOTHING(softmax.getEntailedTightenings(t));
    TS_ASSERT_EQUALS( t.size(), 2u );

    TS_ASSERT( containsTightening(t, Tightening(2, 2, Tightening::LB)) );
    TS_ASSERT( containsTightening(t, Tightening(2, 6, Tightening::UB)) );
  }

  void test_tighten_bounds4()
  {
    QuadraticConstraint softmax (0, 1, 2);

    softmax.notifyLowerBound(0, -2);
    softmax.notifyUpperBound(0, -1);
    softmax.notifyLowerBound(1, 2);
    softmax.notifyUpperBound(1, 4);

    List<Tightening> t;
    TS_ASSERT_THROWS_NOTHING(softmax.getEntailedTightenings(t));
    TS_ASSERT_EQUALS( t.size(), 2u );

    TS_ASSERT( containsTightening(t, Tightening(2, -8, Tightening::LB)) );
    TS_ASSERT( containsTightening(t, Tightening(2, -2, Tightening::UB)) );
  }

  void test_tighten_bounds5()
  {
    QuadraticConstraint softmax (0, 1, 2);

    softmax.notifyLowerBound(0, 1);
    softmax.notifyUpperBound(0, 2);
    softmax.notifyLowerBound(1, 2);
    softmax.notifyUpperBound(1, 4);

    List<Tightening> t;
    TS_ASSERT_THROWS_NOTHING(softmax.getEntailedTightenings(t));
    TS_ASSERT_EQUALS( t.size(), 2u );

    TS_ASSERT( containsTightening(t, Tightening(2, 2, Tightening::LB)) );
    TS_ASSERT( containsTightening(t, Tightening(2, 8, Tightening::UB)) );
  }

  bool containsTightening(const List<Tightening> &ts, Tightening t){
    for ( const auto &ts_ : ts)
    {
      if (ts_._variable == t._variable && ts_._type == t._type &&
          FloatUtils::areEqual(ts_._value, t._value, 0.001))
        return true;
    }
    return false;
  }
};
