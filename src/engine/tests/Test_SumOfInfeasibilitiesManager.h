/*********************                                                        */
/*! \file Test_SumOfInfeasibilitiesManager.h
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
#include "ReluConstraint.h"
#include "MaxConstraint.h"
#include "SumOfInfeasibilitiesManager.h"

class SumOfInfeasibilitiesManagerTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_get_heuristic_cost()
    {
        InputQuery ipq;
        ipq.setNumberOfVariables(6);
        ReluConstraint *relu1 = new ReluConstraint(0,1);
        ReluConstraint *relu2 = new ReluConstraint(2,3);
        ReluConstraint *relu3 = new ReluConstraint(4,5);
        MaxConstraint *max1 = new MaxConstraint(4, {1,3,5});

        ipq.addPiecewiseLinearConstraint(relu1);
        ipq.addPiecewiseLinearConstraint(relu2);
        ipq.addPiecewiseLinearConstraint(relu3);
        ipq.addPiecewiseLinearConstraint(max1);

        for ( unsigned i = 0; i < 6; ++i )
        {
            ipq.setLowerBound( i, -2 );
            ipq.setUpperBound( i, 2 );
        }

        List<PiecewiseLinearConstraint *> plConstraints = {relu1, relu2, relu3, max1};

        for ( const auto &c : plConstraints )
        {
            for ( const auto &var : c->getParticipatingVariables() )
            {
                c->notifyLowerBound( var, -2 );
                c->notifyUpperBound( var, 2 );
            }
        }

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING
            ( soiManager =
              std::unique_ptr<SumOfInfeasibilitiesManager>
              ( new SumOfInfeasibilitiesManager( plConstraints ) ) );
    }
};
