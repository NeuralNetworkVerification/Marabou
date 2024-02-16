/*********************                                                        */
/*! \file Test_Checker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "CSRMatrix.h"
#include "Checker.h"
#include "cxxtest/TestSuite.h"

class CheckerTestSuite : public CxxTest::TestSuite
{
public:
    /*
      Tests certification methods
    */
    void test_certification()
    {
        unsigned m = 3, n = 6;
        double A[] = { 1, 0, -1, 1, 0, 0, 0, -1, 2, 0, 1, 0, 0.5, 0, -1, 0, 0, 1 };

        auto initialTableau = CSRMatrix( A, m, n );

        Vector<double> groundUpperBounds( n, 1 );
        Vector<double> groundLowerBounds( n, 0 );
        groundUpperBounds[5] = 2;

        ReluConstraint relu1 = ReluConstraint( 0, 2 ); // aux var is 4
        ReluConstraint relu2 = ReluConstraint( 1, 3 ); // aux var is 5
        List<PiecewiseLinearConstraint *> constraintsList = { &relu1, &relu2 };

        // Set a complete tree of depth 3, using 2 ReLUs
        auto *root = new UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );

        Checker checker(
            root, m, &initialTableau, groundUpperBounds, groundLowerBounds, constraintsList );

        auto splits1 = relu1.getCaseSplits();
        auto splits2 = relu2.getCaseSplits();
        TS_ASSERT_EQUALS( splits1.size(), 2U );
        TS_ASSERT_EQUALS( splits2.size(), 2U );

        PiecewiseLinearCaseSplit split1_1 = splits1.back();
        PiecewiseLinearCaseSplit split1_2 = splits1.front();

        PiecewiseLinearCaseSplit split2_1 = splits2.back();
        PiecewiseLinearCaseSplit split2_2 = splits2.front();

        TS_ASSERT_EQUALS( split1_2.getBoundTightenings().size(), 2U );

        // Child with missing aux tightening
        auto *child1 = new UnsatCertificateNode( root, split1_1 );
        auto *child2 = new UnsatCertificateNode( root, split1_2 );

        auto *child2_1 = new UnsatCertificateNode( child2, split2_1 );
        auto *child2_2 = new UnsatCertificateNode( child2, split2_2 );

        root->setVisited();
        child2->setVisited();
        child1->setVisited();
        child2_1->setVisited();

        // Visited leaves have no contradiction, so certification will fail
        TS_ASSERT( !checker.check() );

        // Mark visited leaves with flags that immediately certify them, for debugging purpose only
        child1->setSATSolutionFlag();
        child2_1->setDelegationStatus( DelegationStatus::DELEGATE_DONT_SAVE );
        TS_ASSERT( checker.check() );

        // Visited leaf should be checked as well
        // Certification should fail since child2_2 has no contradiction
        child2_2->setVisited();
        TS_ASSERT( !checker.check() );

        delete root;
    }
};
