/*********************                                                        */
/*! \file BoundsExplainer.h
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


#include <cxxtest/TestSuite.h>

#include "BoundsExplainer.h"
#include "context/cdlist.h"
#include "context/context.h"

class BoundsExplainerTestSuite : public CxxTest::TestSuite
{
public:

	void setUp()
	{
	}

	void tearDown()
	{
	}

	/*
	 * Test initialization of BoundsExplainer
	 */
	void testInitialization()
	{
		unsigned varsNum = 3, rowsNum = 5;
		BoundsExplainer be( varsNum, rowsNum );

		TS_ASSERT_EQUALS( be.getRowsNum(), rowsNum );
		TS_ASSERT_EQUALS( be.getVarsNum(), varsNum );

		for ( unsigned i = 0; i < varsNum; ++i )
		{
			TS_ASSERT(be.getExplanation( i, true ).empty()  );
			TS_ASSERT(be.getExplanation( i, false ).empty() );
		}
	}

	/*
	 * Test explanation injection
	 */
	void testExplanationInjection()
	{
		unsigned varsNum = 2, rowsNum = 2;
		double value = -2.55;
		BoundsExplainer be( varsNum, rowsNum );

		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( varsNum, value ), 0, true ) );
		auto expl = be.getExplanation( 0, true );

		for ( auto num : expl )
			TS_ASSERT_EQUALS( num, value );
	}

	/*
	 * Test addition of an explanation of the new variable, and correct updates of all previous explanations
	 */
	void testVariableAddition()
	{
		unsigned varsNum = 2, rowsNum = 2;
		BoundsExplainer be( varsNum, rowsNum );
		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( varsNum, 1 ), varsNum - 1, true ) );
		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( varsNum, 5 ), varsNum - 1, false ) );
		be.addVariable();

		TS_ASSERT_EQUALS(be.getRowsNum(), rowsNum + 1 );
		TS_ASSERT_EQUALS(be.getVarsNum(), varsNum + 1 );

		for ( unsigned i = 0; i < varsNum; ++ i)
		{
			TS_ASSERT(be.getExplanation( i, true ).empty() || ( be.getExplanation( i, true ).back() == 0 && be.getExplanation( i, true ).size() == varsNum + 1) );
			TS_ASSERT(be.getExplanation( i, false ).empty() || ( be.getExplanation( i, false ).back() == 0 && be.getExplanation( i, false ).size() == varsNum + 1) );
		}

		TS_ASSERT(be.getExplanation( varsNum, true ).empty() );
		TS_ASSERT(be.getExplanation( varsNum, false ).empty() ) ;
	}

	/*
	 * Test explanation reset
	 */
	void testExplanationReset()
	{
		unsigned varsNum = 1, rowsNum = 1;
		BoundsExplainer be( varsNum, rowsNum );

		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( rowsNum, 1 ), 0, true ) );
		TS_ASSERT( !be.getExplanation( 0 , true ).empty() );

		be.resetExplanation(0, true );
		TS_ASSERT( be.getExplanation( 0 , true ).empty() );
	}

	/*
	 * Test main functionality of BoundsExplainer i.e. updating explanations according to tableau rows
	 */
	void testExplanationUpdates()
	{
		unsigned varsNum = 6, rowsNum = 3;
		BoundsExplainer be( varsNum, rowsNum );
		std::vector<double> row1 { 1, 0, 0 };
		std::vector<double> row2 { 0, -1, 0 };
		std::vector<double> row3 { 0, 0, 2.5 };

		TableauRow updateTableauRow( 6 );
		// row1 + row2 :=  x2 = x0 + 2 x1 - x3 + x4
		// Equivalently x3 = x0 + 2 x1 - x2 - x3 + x4
		// Equivalently x1 = -0.5 x0 + 0.5 x2 + 0.5 x3 - 0.5 x4
		// Rows coefficients are { -1, 1, 0 }
		updateTableauRow._scalar = 0;
		updateTableauRow._lhs = 2;
		updateTableauRow._row[0] = TableauRow::Entry( 0, 1 );
		updateTableauRow._row[1] = TableauRow::Entry( 1, 2 );
		updateTableauRow._row[2] = TableauRow::Entry( 3, -1 );
		updateTableauRow._row[3] = TableauRow::Entry( 4, 1 );
		updateTableauRow._row[4] = TableauRow::Entry( 5, 0 );

		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( row1, 0, true ) );
		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( row2, 1, true ) );
		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( row3, 1, false ) ); // Will not be possible in an actual tableau

		be.updateBoundExplanation( updateTableauRow, true );
		// Result is { 1, 0, 0 } + 2 * { 0, -1, 0 } + { -1, 1, 0}
		std::vector<double> res1 { 0, -1, 0 };
		TS_ASSERT_EQUALS( be.getExplanation( 2, true ), res1 );

		be.updateBoundExplanation( updateTableauRow, false, 3 );
		// Result is 2 * { 0, 0, 2.5 } + { -1, 1, 0 }
		std::vector<double> res2 { -1, 2, 5 };
		TS_ASSERT_EQUALS( be.getExplanation( 3, false ), res2 );

		be.updateBoundExplanation( updateTableauRow, false, 1 );
		// Result is -0.5 * { 1, 0, 0 } + 0.5 * { -1, 2, 5 } - 0.5 * { -1, 1, 0 }
		std::vector<double> res3 { -0.5, 0.5, 2.5 };
		TS_ASSERT_EQUALS( be.getExplanation( 1, false ), res3 );

		// row3:= x1 = x5
		// Rows coefficients are { 0, 0, -2.5 }
		SparseUnsortedList updateSparseRow( 0 );
		updateSparseRow.append( 1, -2.5 );
		updateSparseRow.append( 5, -2.5 );

		be.updateBoundExplanationSparse( updateSparseRow, true, 5 );
		// Result is  ( 1 / 2.5 ) * ( -2.5 ) * { -0.5, 0.5, 2.5 } + ( 1 / 2.5 ) * { 0, 0, -2.5 }
		std::vector<double> res4 { 0.5, -0.5, -3.5 };
		TS_ASSERT_EQUALS( be.getExplanation( 5, true ), res4 );
	}

	/*
	 * Test deep copying of BoundsExplainer using operator=
	 */
	void testCopying()
	{
		unsigned varsNum = 2, rowsNum = 2;
		BoundsExplainer be( varsNum, rowsNum );

		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( rowsNum, 1 ), varsNum - 1, true ) );
		TS_ASSERT_THROWS_NOTHING( be.injectExplanation( std::vector<double>( rowsNum, 5 ), varsNum - 1, false ) );

		BoundsExplainer beCopy( 1, 1 );
		beCopy = be;

		TS_ASSERT_EQUALS( beCopy.getRowsNum(), be.getRowsNum() );
		TS_ASSERT_EQUALS( beCopy.getVarsNum(), be.getVarsNum() );

		for ( unsigned i = 0; i < varsNum; ++i)
		{
			auto upperExpl = be.getExplanation( i, true );
			auto lowerExpl = be.getExplanation( i, false );

			auto upperExplCopy = beCopy.getExplanation( i, true );
			auto lowerExplCopy = beCopy.getExplanation( i, false );

			TS_ASSERT_EQUALS( upperExpl, upperExplCopy );
			TS_ASSERT_EQUALS( lowerExpl, lowerExplCopy );
		}
	}
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
