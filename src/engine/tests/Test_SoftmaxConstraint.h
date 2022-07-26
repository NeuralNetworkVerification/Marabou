/*********************                                                        */
/*! \file Test_SigmoidConstraint.h
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

#include "InputQuery.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include <string.h>

class SoftMaxConstraintTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_softmax_constraint_construction()
    {
        List<unsigned> inputs = { 1, 2, 3 };
        List<unsigned> outputs = { 4, 5, 6 };

        InputQuery ipq;
        ipq.setNumberOfVariables(7);
        TS_ASSERT_THROWS_NOTHING( ipq.addSoftmaxConstraint(inputs, outputs) );
        TS_ASSERT_EQUALS( ipq.getNonlinearConstraints().size(), 6u + 3u );
        ipq.markInputVariable(1, 0 );
        ipq.markInputVariable(2, 1 );
        ipq.markInputVariable(3, 2 );
        TS_ASSERT( ipq.constructNetworkLevelReasoner() );
        NLR::NetworkLevelReasoner *nlr = ipq.getNetworkLevelReasoner();

        // Should have four layers
        // eDiff_1 = x2 - x1
        // eDiff_out_1 = e^(eDiff_1)
        // eDiffSum = eDiff_out_1 + ...
        // x4 = recip (eDiffSum)
        TS_ASSERT_EQUALS(nlr->getNumberOfLayers(), 5u);

        double inputArr[3];
        inputArr[0] = 3;
        inputArr[1] = 3;
        inputArr[2] = 3;

        double outputArr[3];
        TS_ASSERT_THROWS_NOTHING(nlr->evaluate(inputArr, outputArr));
        TS_ASSERT( FloatUtils::areEqual(outputArr[0], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[1], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[2], 0.333333333333333));

        inputArr[0] = 1;
        inputArr[1] = 3;
        inputArr[2] = 5;

        TS_ASSERT_THROWS_NOTHING(nlr->evaluate(inputArr, outputArr));
        TS_ASSERT( FloatUtils::areEqual(outputArr[0], 0.015876239976467));
        TS_ASSERT( FloatUtils::areEqual(outputArr[1], 0.1173104278262));
        TS_ASSERT( FloatUtils::areEqual(outputArr[2], 0.86681333219734));

    }

  void test_softmax_constraint_construction_2()
  {
        List<unsigned> inputs = { 1, 2, 3 };
        List<unsigned> outputs = { 4, 5, 6 };


        InputQuery ipq;
        ipq.setNumberOfVariables(13);
        TS_ASSERT_THROWS_NOTHING( ipq.addSoftmaxConstraint(inputs, outputs) );

        inputs = { 7, 8, 9 };
        outputs = { 10, 11, 12 };
        TS_ASSERT_THROWS_NOTHING( ipq.addSoftmaxConstraint(inputs, outputs) );

        TS_ASSERT_EQUALS( ipq.getNonlinearConstraints().size(), 18u );
        ipq.markInputVariable(1, 0 );
        ipq.markInputVariable(2, 1 );
        ipq.markInputVariable(3, 2 );
        ipq.markInputVariable(7, 3 );
        ipq.markInputVariable(8, 4 );
        ipq.markInputVariable(9, 5 );
        TS_ASSERT( ipq.constructNetworkLevelReasoner() );
        NLR::NetworkLevelReasoner *nlr = ipq.getNetworkLevelReasoner();

        // Should have four layers
        // eDiff_1 = x2 - x1
        // eDiff_out_1 = e^(eDiff_1)
        // eDiffSum = eDiff_out_1 + ...
        // x4 = recip (eDiffSum)
        TS_ASSERT_EQUALS(nlr->getNumberOfLayers(), 5u);

        double inputArr[6];
        inputArr[0] = 3;
        inputArr[1] = 3;
        inputArr[2] = 3;
        inputArr[3] = 3;
        inputArr[4] = 3;
        inputArr[5] = 3;

        double outputArr[6];
        TS_ASSERT_THROWS_NOTHING(nlr->evaluate(inputArr, outputArr));
        TS_ASSERT( FloatUtils::areEqual(outputArr[0], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[1], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[2], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[3], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[4], 0.333333333333333));
        TS_ASSERT( FloatUtils::areEqual(outputArr[5], 0.333333333333333));

        inputArr[0] = 1;
        inputArr[1] = 3;
        inputArr[2] = 5;
        inputArr[3] = 1;
        inputArr[4] = 3;
        inputArr[5] = 5;

        TS_ASSERT_THROWS_NOTHING(nlr->evaluate(inputArr, outputArr));
        TS_ASSERT( FloatUtils::areEqual(outputArr[0], 0.015876239976467));
        TS_ASSERT( FloatUtils::areEqual(outputArr[1], 0.1173104278262));
        TS_ASSERT( FloatUtils::areEqual(outputArr[2], 0.86681333219734));
        TS_ASSERT( FloatUtils::areEqual(outputArr[3], 0.015876239976467));
        TS_ASSERT( FloatUtils::areEqual(outputArr[4], 0.1173104278262));
        TS_ASSERT( FloatUtils::areEqual(outputArr[5], 0.86681333219734));

        for ( unsigned i = 0; i < ipq.getNumberOfVariables(); ++i )
        {
          ipq.setLowerBound(i, FloatUtils::negativeInfinity());
          ipq.setUpperBound(i, FloatUtils::infinity());
        }

        for ( unsigned i = 0; i < 3; ++i )
        {
          ipq.setLowerBound(ipq.inputVariableByIndex(i), 3);
          ipq.setUpperBound(ipq.inputVariableByIndex(i), 3);
        }

        ipq.setLowerBound(ipq.inputVariableByIndex(3), 1);
        ipq.setUpperBound(ipq.inputVariableByIndex(3), 1);
        ipq.setLowerBound(ipq.inputVariableByIndex(4), 3);
        ipq.setUpperBound(ipq.inputVariableByIndex(4), 3);
        ipq.setLowerBound(ipq.inputVariableByIndex(5), 5);
        ipq.setUpperBound(ipq.inputVariableByIndex(5), 5);

        TS_ASSERT_THROWS_NOTHING( nlr->obtainCurrentBounds(ipq) );

        TS_ASSERT_THROWS_NOTHING( nlr->deepPolyPropagation() );

        List<Tightening> tightenings;
        nlr->getConstraintTightenings( tightenings );

        TS_ASSERT(hasLowerBound( tightenings, 4, 0.333333333333333) );
        TS_ASSERT(hasUpperBound( tightenings, 4, 0.333333333333333) );
        TS_ASSERT(hasLowerBound( tightenings, 5, 0.333333333333333) );
        TS_ASSERT(hasUpperBound( tightenings, 5, 0.333333333333333) );
        TS_ASSERT(hasLowerBound( tightenings, 6, 0.333333333333333) );
        TS_ASSERT(hasUpperBound( tightenings, 6, 0.333333333333333) );

        TS_ASSERT(hasLowerBound( tightenings, 10, 0.015876239976467) );
        TS_ASSERT(hasUpperBound( tightenings, 10, 0.015876239976467) );
        TS_ASSERT(hasLowerBound( tightenings, 11, 0.1173104278262) );
        TS_ASSERT(hasUpperBound( tightenings, 11, 0.1173104278262) );
        TS_ASSERT(hasLowerBound( tightenings, 12, 0.86681333219734) );
        TS_ASSERT(hasUpperBound( tightenings, 12, 0.86681333219734) );
    }

  bool hasLowerBound( List<Tightening> tightenings, unsigned variable,
                           double bound )
  {
    for( const auto &t : tightenings )
      {
        if ( t._variable == variable && t._type == Tightening::LB )
          return FloatUtils::areEqual( t._value, bound );
      }
    return false;
  }

  bool hasUpperBound( List<Tightening> tightenings, unsigned variable,
                      double bound )
  {
    for( const auto &t : tightenings )
      {
        if ( t._variable == variable && t._type == Tightening::UB )
          return FloatUtils::areEqual( t._value, bound );
      }
    return false;
  }

};
