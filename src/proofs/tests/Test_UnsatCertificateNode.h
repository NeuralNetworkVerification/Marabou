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
#include "UnsatCertificateNode.h"
#include "context/cdlist.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>

class UnsatCertificateNodeTestSuite : public CxxTest::TestSuite
{
public:
    /*
      Tests a simple tree construction
    */
    void test_tree_relations()
    {
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

        TS_ASSERT( root->getChildBySplit( split1 ) == nullptr );
        TS_ASSERT( root->getChildBySplit( split2 ) == nullptr );

        delete root;
    }

    /*
      Tests methods that set and get the contradiction
     */
    void test_contradiction()
    {
        UnsatCertificateNode root = UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );

        auto upperBoundExplanation = Vector<double>( 1, 1 );
        auto lowerBoundExplanation = Vector<double>( 1, 1 );

        auto *contradiction = new Contradiction( Vector<double>( 1, 0 ) );
        root.setContradiction( contradiction );
        TS_ASSERT_EQUALS( root.getContradiction(), contradiction );
    }

    /*
      Tests changes in PLC Explanations list
    */
    void test_plc_explanation_changes()
    {
        UnsatCertificateNode root = UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );
        Vector<SparseUnsortedList> emptyVec;

        auto explanation1 = std::shared_ptr<PLCLemma>(
            new PLCLemma( { 1 }, 1, 0, Tightening::UB, Tightening::UB, emptyVec, RELU ) );
        auto explanation2 = std::shared_ptr<PLCLemma>(
            new PLCLemma( { 1 }, 1, -1, Tightening::UB, Tightening::UB, emptyVec, RELU ) );
        auto explanation3 = std::shared_ptr<PLCLemma>(
            new PLCLemma( { 1 }, 1, -4, Tightening::UB, Tightening::UB, emptyVec, RELU ) );

        TS_ASSERT( root.getPLCLemmas().empty() );

        root.addPLCLemma( explanation1 );
        root.addPLCLemma( explanation2 );
        root.addPLCLemma( explanation3 );
        TS_ASSERT_EQUALS( root.getPLCLemmas().size(), 3U );

        root.deletePLCExplanations();
        TS_ASSERT( root.getPLCLemmas().empty() );
    }
};
