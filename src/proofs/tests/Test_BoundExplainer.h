/*********************                                                        */
/*! \file Test_BoundExplainer.h
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
#include "context/cdo.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>

using CVC4::context::Context;
using namespace CVC4::context;

class BoundsExplainerTestSuite : public CxxTest::TestSuite
{
public:
    Context *context;

    void setUp()
    {
        TS_ASSERT_THROWS_NOTHING( context = new Context );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete context; );
    }

    /*
      Test initialization of BoundExplainer
    */
    void test_initialization()
    {
        unsigned numberOfVariables = 3;
        unsigned numberOfRows = 5;
        BoundExplainer be( numberOfVariables, numberOfRows, *context );

        TS_ASSERT_EQUALS( be.getNumberOfRows(), numberOfRows );
        TS_ASSERT_EQUALS( be.getNumberOfVariables(), numberOfVariables );

        for ( unsigned i = 0; i < numberOfVariables; ++i )
        {
            TS_ASSERT( be.isExplanationTrivial( i, true ) );
            TS_ASSERT( be.isExplanationTrivial( i, false ) );
        }
    }

    /*
      Test setExplanation
    */
    void test_set_explanation()
    {
        unsigned numberOfVariables = 2;
        unsigned numberOfRows = 2;
        double value = -2.55;
        BoundExplainer be( numberOfVariables, numberOfRows, *context );

        TS_ASSERT_THROWS_NOTHING(
            be.setExplanation( Vector<double>( numberOfVariables, value ), 0, true ) );
        auto explanation = be.getExplanation( 0, true );

        for ( const auto &entry : explanation )
            TS_ASSERT_EQUALS( entry._value, value );

        TS_ASSERT_EQUALS( explanation.getSize(), numberOfVariables );
    }

    /*
      Test addition of an explanation of the new variable, and correct updates of all previous
      explanations
    */
    void test_variable_addition()
    {
        unsigned numberOfVariables = 2;
        unsigned numberOfRows = 2;
        BoundExplainer be( numberOfVariables, numberOfRows, *context );

        TS_ASSERT_THROWS_NOTHING( be.setExplanation(
            Vector<double>( numberOfVariables, 1 ), numberOfVariables - 1, true ) );
        TS_ASSERT_THROWS_NOTHING( be.setExplanation(
            Vector<double>( numberOfVariables, 5 ), numberOfVariables - 1, false ) );
        be.addVariable();

        TS_ASSERT_EQUALS( be.getNumberOfRows(), numberOfRows + 1 );
        TS_ASSERT_EQUALS( be.getNumberOfVariables(), numberOfVariables + 1 );

        for ( unsigned i = 0; i < numberOfVariables; ++i )
        {
            // the sizes of explanations should not change
            TS_ASSERT( be.isExplanationTrivial( i, true ) ||
                       be.getExplanation( i, true ).getSize() == numberOfVariables );
            TS_ASSERT( be.isExplanationTrivial( i, false ) ||
                       be.getExplanation( i, false ).getSize() == numberOfVariables );
        }

        TS_ASSERT( be.isExplanationTrivial( numberOfVariables, true ) );
        TS_ASSERT( be.isExplanationTrivial( numberOfVariables, false ) );
    }

    /*
      Test explanation reset
    */
    void test_explanation_reset()
    {
        unsigned numberOfVariables = 1;
        unsigned numberOfRows = 1;
        BoundExplainer be( numberOfVariables, numberOfRows, *context );

        TS_ASSERT_THROWS_NOTHING( be.setExplanation( Vector<double>( numberOfRows, 1 ), 0, true ) );
        TS_ASSERT( !be.isExplanationTrivial( 0, true ) );

        be.resetExplanation( 0, true );
        TS_ASSERT( be.isExplanationTrivial( 0, true ) );
    }

    /*
      Test main functionality of BoundExplainer i.e. updating explanations according to tableau rows
    */
    void test_explanation_updates()
    {
        unsigned numberOfVariables = 6;
        unsigned numberOfRows = 3;
        BoundExplainer be( numberOfVariables, numberOfRows, *context );
        Vector<double> row1{ 1, 0, 0 };
        Vector<double> row2{ 0, -1, 0 };
        Vector<double> row3{ 0, 0, 2.5 };

        TableauRow updateTableauRow( 6 );
        // row1 + row2 :=  x2 = x0 + 2 x1 - x3 + x4
        // Equivalently x3 = x0 + 2 x1 - x2 - x3 + x4
        // Row coefficients are { -1, 1, 0 }
        // Equivalently x1 = -0.5 x0 + 0.5 x2 + 0.5 x3 - 0.5 x4
        // Row coefficients are { 1, -1, 0 }
        updateTableauRow._scalar = 0;
        updateTableauRow._lhs = 2;
        updateTableauRow._row[0] = TableauRow::Entry( 0, 1 );
        updateTableauRow._row[1] = TableauRow::Entry( 1, 2 );
        updateTableauRow._row[2] = TableauRow::Entry( 3, -1 );
        updateTableauRow._row[3] = TableauRow::Entry( 4, 1 );
        updateTableauRow._row[4] = TableauRow::Entry( 5, 0 );

        TS_ASSERT_THROWS_NOTHING( be.setExplanation( row1, 0, true ) );
        TS_ASSERT_THROWS_NOTHING( be.setExplanation( row2, 1, true ) );
        TS_ASSERT_THROWS_NOTHING( be.setExplanation( row3, 1, false ) ); // Will not be possible in
                                                                         // an actual tableau

        be.updateBoundExplanation( updateTableauRow, true );
        // Result is { 1, 0, 0 } + 2 * { 0, -1, 0 } + { 1, -1, 0}
        Vector<double> res1{ 2, -3, 0 };

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( be.getExplanation( 2, true ).get( i ), res1[i] );

        be.updateBoundExplanation( updateTableauRow, false, 3 );
        // Result is 2 * { 0, 0, 2.5 } + { -1, 1, 0 }
        Vector<double> res2{ -1, 2, 5 };
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( be.getExplanation( 3, false ).get( i ), res2[i] );

        be.updateBoundExplanation( updateTableauRow, false, 1 );
        // Result is -0.5 * { 1, 0, 0 } + 0.5 * { -1, 2, 5 } - 0.5 * { 1, -1, 0 }
        Vector<double> res3{ -1.5, 1.5, 2.5 };

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( be.getExplanation( 1, false ).get( i ), res3[i] );

        // row3:= x1 = x5
        // Row coefficients are { 0, 0, 2.5 }
        SparseUnsortedList updateSparseRow( 0 );
        updateSparseRow.append( 1, -2.5 );
        updateSparseRow.append( 5, -2.5 );

        be.updateBoundExplanationSparse( updateSparseRow, true, 5 );
        // Result is  ( 1 / 2.5 ) * ( -2.5 ) * { -1.5, 1.5, 2.5 } + ( 1 / 2.5 ) * { 0, 0, 2.5 }
        Vector<double> res4{ 1.5, -1.5, -1.5 };
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( be.getExplanation( 5, true ).get( i ), res4[i] );
    }
};
