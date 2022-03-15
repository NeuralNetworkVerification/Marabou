/*********************                                                        */
/*! \file Test_UnsatCertificateNode.h
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

#include "BoundExplainer.h"
#include "context/cdlist.h"
#include "context/context.h"
#include <cxxtest/TestSuite.h>
#include "UnsatCertificateNode.h"

class UnsatCertificateNodeTestSuite : public CxxTest::TestSuite
{
public:
    /*
      Tests a simple tree construction
    */
    void testTreeRelations()
    {
        Vector<double> row1 = { 1, 0, -1, 1, 0, 0 };
        Vector<double> row2 = { 0, -1, 2, 0, 1, 0 };
        Vector<double> row3 = { 0.5, 0, -1, 0, 0, 1 };
        Vector<Vector<double>> initialTableau = { row1, row2, row3 };

        Vector<double> groundUpperBounds( 6, 1 );
        Vector<double> groundLowerBounds( 6, 0 );

        auto *root = new UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );

        ReluConstraint relu = ReluConstraint( 1, 3 );
        auto splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        PiecewiseLinearCaseSplit split1 = splits.back();
        PiecewiseLinearCaseSplit split2 = splits.front();

        auto *child1 = new UnsatCertificateNode( root, split1 );
        auto *child2 = new UnsatCertificateNode( root, split2 );

        TS_ASSERT_EQUALS( child1->getParent(), root );
        TS_ASSERT_EQUALS( child2->getParent(), root );

        TS_ASSERT_EQUALS( root->getChildBySplit( split1 ), child1 );
        TS_ASSERT_EQUALS( root->getChildBySplit( split2 ), child2 );

        TS_ASSERT_EQUALS( child1->getSplit(), split1 );
        TS_ASSERT_EQUALS( child2->getSplit(), split2 );

        root->makeLeaf();

        TS_ASSERT_EQUALS( root->getChildBySplit( split1 ), nullptr );
        TS_ASSERT_EQUALS( root->getChildBySplit( split2 ), nullptr );

        delete root;
    }

    /*
      Tests methods that set and get the contradiction
     */
    void testContradiction()
    {
        Vector<Vector<double>> initialTableau = { Vector<double>( 1,1 ) };
        Vector<double> groundUpperBounds( 1, 1 );
        Vector<double> groundLowerBounds( 1, 0 );

        UnsatCertificateNode root = UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );
        auto upperBoundExplanation = Vector<double>(1, 1);

        auto lowerBoundExplanation = Vector<double>(1, 1);

        auto *contradiction = new Contradiction( 0, upperBoundExplanation, lowerBoundExplanation );
        root.setContradiction( contradiction );
        TS_ASSERT_EQUALS( root.getContradiction(), contradiction );
    }

    /*
      Tests changes in PLC Explanations list
    */
    void testPLCExplChanges()
    {
        Vector<Vector<double>> initialTableau = { Vector<double>( 1,1 ) };
        Vector<double> groundUpperBounds( 1, 1 );
        Vector<double> groundLowerBounds( 1, -1 );

        UnsatCertificateNode root = UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );
        Vector<double> emptyVec;

        auto explanation1 = std::shared_ptr<PLCExplanation>( new PLCExplanation( 1, 1, 0, UPPER, UPPER, emptyVec, RELU, 0 ) );
        auto explanation2 = std::shared_ptr<PLCExplanation>( new PLCExplanation( 1, 1, -1, UPPER, UPPER, emptyVec, RELU, 1 ) );
        auto explanation3 = std::shared_ptr<PLCExplanation>( new PLCExplanation( 1, 1, -4, UPPER, UPPER, emptyVec, RELU, 2 ) );

        TS_ASSERT( root.getPLCExplanations().empty() );

        root.addPLCExplanation( explanation1 );
        root.addPLCExplanation( explanation2 );
        root.addPLCExplanation( explanation3 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), 3U );

        root.removePLCExplanationsBelowDecisionLevel( 0 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), 2U );
        TS_ASSERT_EQUALS( root.getPLCExplanations().front(), explanation2 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().back(), explanation3 );

        root.deletePLCExplanations();
        TS_ASSERT( root.getPLCExplanations().empty() );

        List<std::shared_ptr<PLCExplanation>> list = { explanation1 };
        root.setPLCExplanations( list );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), 1U );
        TS_ASSERT_EQUALS( root.getPLCExplanations().front(), explanation1 );
    }
};
