/*********************                                                        */
/*! \file BoundExplainer.h
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
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
#include "UNSATCertificate.h"

class CertificateNodeTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    /*
      Tests a simple tree construction
    */
    void testTreeRelations()
    {
        Vector<double> row1 = { 1, 0, -1, 1, 0, 0 };
        Vector<double> row2 = { 0, -1, 2, 0, 1, 0 };
        Vector<double> row3 = { 0.5, 0, -1, 0, 0, 1 };
        Vector<Vector<double>> initialTableau = {row1, row2, row3 };

        Vector<double> groundUpperBounds( 6, 1 );
        Vector<double> groundLowerBounds( 6, 0 );

        auto *root = new CertificateNode( &initialTableau, groundUpperBounds, groundLowerBounds );

        ReluConstraint relu = ReluConstraint(1, 3);
        auto splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), ( unsigned ) 2 );

        PiecewiseLinearCaseSplit split1 = splits.back();
        PiecewiseLinearCaseSplit split2 = splits.front();

        auto *child1 = new CertificateNode( root, split1 );
        auto *child2 = new CertificateNode( root, split2 );

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

        CertificateNode root = CertificateNode( &initialTableau, groundUpperBounds, groundLowerBounds );
        auto upperBoundExplanation = new double [1];
        upperBoundExplanation[0] = 1;

        auto lowerBoundExplanation = new double [1];
        lowerBoundExplanation[0] = -1;

        auto *contradiction = new Contradiction { 0, upperBoundExplanation, lowerBoundExplanation };
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
        Vector<double> groundLowerBounds( 1, 0 );

        CertificateNode root = CertificateNode( &initialTableau, groundUpperBounds, groundLowerBounds );

        auto *explanation1 = new PLCExplanation { 1, 1, 0, true, true, NULL, RELU, 0 };
        auto *explanation2 = new PLCExplanation {1, 1, -1, true, true, NULL, RELU, 1 };
        auto *explanation3 = new PLCExplanation { 1, 1, -4, true, true, NULL, RELU, 2 };

        TS_ASSERT( root.getPLCExplanations().empty() );

        root.addPLCExplanation( explanation1 );
        root.addPLCExplanation( explanation2 );
        root.addPLCExplanation( explanation3 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 3 );

        root.removePLCExplanationsBelowDecisionLevel( 0 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 2 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().front(), explanation2 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().back(), explanation3 );

        root.resizePLCExplanationsList( 1 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 1 );
        TS_ASSERT_EQUALS( root.getPLCExplanations().back(), explanation2 );

        root.deletePLCExplanations();
        TS_ASSERT( root.getPLCExplanations().empty() );

        // removePLCExplanationsBelowDecisionLevel doesn't delete by itself
        delete explanation1;
    }

    /*
       Tests certification methods
     */
    void testCertification()
    {
        Vector<double> row1 = { 1, 0, -1, 0, 1, 0, 0 }; // Row of ReLU1
        Vector<double> row2 = { 0, 1, 0, -1, 0, 1, 0 }; // Row of ReLU2
        Vector<double> row3 = { 0.5, 0, -1, 0, 0, 0, 1 };
        Vector<Vector<double>> initialTableau = { row1, row2, row3 };

        Vector<double> groundUpperBounds( row1.size(), 1 );
        Vector<double> groundLowerBounds( row1.size(), 0 );
        groundUpperBounds[6] = 2;

        // Set a complete tree of depth 3, using 2 ReLUs
        auto *root = new CertificateNode( &initialTableau, groundUpperBounds, groundLowerBounds );

        ReluConstraint relu1 = ReluConstraint( 0, 2 ); // aux var is 4
        ReluConstraint relu2 = ReluConstraint( 1, 3) ; // aux var is 5

        root->addProblemConstraint( RELU, { 0, 2, 4 }, PHASE_NOT_FIXED );
        root->addProblemConstraint( RELU, { 1, 3, 5 }, PHASE_NOT_FIXED );

        auto splits1 = relu1.getCaseSplits();
        auto splits2 = relu2.getCaseSplits();
        TS_ASSERT_EQUALS( splits1.size(), ( unsigned ) 2 );
        TS_ASSERT_EQUALS( splits2.size(), ( unsigned ) 2 );

        PiecewiseLinearCaseSplit split1_1 = splits1.back();
        PiecewiseLinearCaseSplit split1_2 = splits1.front();

        PiecewiseLinearCaseSplit split2_1 = splits2.back();
        PiecewiseLinearCaseSplit split2_2 = splits2.front();

        TS_ASSERT_EQUALS( split1_2.getBoundTightenings().size(), ( unsigned ) 2 );

        auto *child1 = new CertificateNode( root, split1_1 ); // Child with missing aux tightening
        auto *child2 = new CertificateNode( root, split1_2 );

        auto *child2_1 = new CertificateNode( child2, split2_1 );
        auto *child2_2 = new CertificateNode( child2, split2_2 );

        root->wasVisited();
        child2->wasVisited();
        child1->wasVisited();
        child2_1->wasVisited();

        // Visited leaves have no contradiction, so certification will fail
        TS_ASSERT( !root->certify() );

        // Mark visited leaves with flags that immediately certify them
        child1->hasSATSolution();
        child2_1->shouldDelegate( 0, DelegationStatus::DELEGATE_DONT_SAVE );
        TS_ASSERT( root->certify() );

        // Visited leaf should be checked as well
        // Certification should fail since child2_2 has no contradiction
        child2_2->wasVisited();
        TS_ASSERT(!root->certify() );

        delete root;
    }
};
//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:

