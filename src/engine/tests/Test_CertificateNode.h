/*********************                                                        */
/*! \file BoundsExplainer.h
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/


#include <cxxtest/TestSuite.h>

#include "BoundsExplainer.h"
#include "context/cdlist.h"
#include "context/context.h"
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

	void testTreeRelations()
	{
		std::vector<double> row1 = { 1, 0, -1, 1, 0, 0 };
		std::vector<double> row2 = { 0, -1, 2, 0, 1, 0 };
		std::vector<double> row3 = { 0.5, 0, -1, 0, 0, 1 };
		std::vector<std::vector<double>> it = { row1, row2, row3 };

		std::vector<double> ugb( 6, 1 );
		std::vector<double> lgb( 6, 0 );

		CertificateNode *root = new CertificateNode( &it, ugb, lgb );

		ReluConstraint relu = ReluConstraint(1, 3);
		auto splits = relu.getCaseSplits();
		TS_ASSERT_EQUALS( splits.size(), ( unsigned ) 2 );

		PiecewiseLinearCaseSplit split1 = splits.back();
		PiecewiseLinearCaseSplit split2 = splits.front();

		CertificateNode *child1 = new CertificateNode( root, split1 );
		CertificateNode *child2 = new CertificateNode( root, split2 );

		TS_ASSERT_EQUALS( child1->getParent(), root );
		TS_ASSERT_EQUALS( child2->getParent(), root );

		TS_ASSERT_EQUALS( root->getChildBySplit( split1 ), child1 );
		TS_ASSERT_EQUALS( root->getChildBySplit( split2 ), child2 );

		TS_ASSERT_EQUALS( child1->getSplit(), split1 );
		TS_ASSERT_EQUALS( child2->getSplit(), split2 );

		root->makeLeaf();

		TS_ASSERT_EQUALS(root->getChildBySplit( split1 ), nullptr );
		TS_ASSERT_EQUALS(root->getChildBySplit( split2 ), nullptr );

		delete root;
	}

	void testContradiction()
	{
		std::vector<std::vector<double>> it = { std::vector<double>( 1,1 ) };
		std::vector<double> ugb( 1, 1 );
		std::vector<double> lgb( 1, 0 );

		CertificateNode root = CertificateNode( &it, ugb, lgb );
		auto uexpl = new double [1];
		uexpl[0] = 1;

		auto lexpl = new double [1];
		lexpl[0] = -1;

		Contradiction* contra = new Contradiction { 0, uexpl, lexpl };
		root.setContradiction( contra );
		TS_ASSERT_EQUALS( root.getContradiction(), contra );
	}

	void testPLCExplChanges()
	{
		std::vector<std::vector<double>> it = { std::vector<double>( 1,1 ) };
		std::vector<double> ugb( 1, 1 );
		std::vector<double> lgb( 1, 0 );

		CertificateNode root = CertificateNode( &it, ugb, lgb );

		PLCExplanation *expl1 = new PLCExplanation { 1, 1, 0, true, true, NULL, RELU, 0 };
		PLCExplanation *expl2 = new PLCExplanation { 1, 1, -1, true, true, NULL, RELU, 1 };
		PLCExplanation *expl3 = new PLCExplanation { 1, 1, -4, true, true, NULL, RELU, 2 };

		TS_ASSERT( root.getPLCExplanations().empty() );

		root.addPLCExplanation( expl1 );
		root.addPLCExplanation( expl2 );
		root.addPLCExplanation( expl3 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 3 );

		root.removePLCExplanationsBelowDecisionLevel( 0 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 2 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().front(), expl2 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().back(), expl3 );

		root.resizePLCExplanationsList( 1 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().size(), ( unsigned ) 1 );
		TS_ASSERT_EQUALS( root.getPLCExplanations().back(), expl2 );

		root.deletePLCExplanations();
		TS_ASSERT( root.getPLCExplanations().empty() );

		delete expl1; // removePLCExplanationsBelowDecisionLevel doesn't delete by itself
	}

	void testCertification()
	{
		std::vector<double> row1 = { 1, 0, -1, 0, 1, 0, 0 }; // Row of ReLU1
		std::vector<double> row2 = { 0, 1, 0, -1, 0, 1, 0 }; // Row of ReLU2
		std::vector<double> row3 = { 0.5, 0, -1, 0, 0, 0, 1 };
		std::vector<std::vector<double>> it = { row1, row2, row3 };

		std::vector<double> ugb( row1.size(), 1 );
		std::vector<double> lgb( row1.size(), 0 );
		ugb[6] = 2;

		// Set a complete tree of depth 3, using 2 ReLUs
		CertificateNode *root = new CertificateNode( &it, ugb, lgb );

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

		CertificateNode *child1 = new CertificateNode( root, split1_1 ); // Child with missing aux tightening
		CertificateNode *child2 = new CertificateNode( root, split1_2 );

		CertificateNode *child2_1 = new CertificateNode( child2, split2_1 );
		CertificateNode *child2_2 = new CertificateNode( child2, split2_2 );

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

