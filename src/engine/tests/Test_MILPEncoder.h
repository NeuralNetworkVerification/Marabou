/*********************                                                        */
/*! \file Test_MILPEncoder.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MILPEncoder.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include "Query.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MILPEncoderTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_encode_max_constraint()
    {
#ifdef ENABLE_GUROBI
        unsigned f;

        // elements = (x1, x2, x3)
        Set<unsigned> elements;
        for ( unsigned i = 1; i < 4; ++i )
            elements.insert( i );

        //
        // x1 = max(x1, x2, x3)
        // Bounds:
        //   100 <= x0 <= 100
        //   0 <= x1 <= 2
        //   1 <= x2 <= 2
        //   3 <= x3 <= 4
        //
        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 4 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 2, 6 );

        // 100 <= x0 <= 100
        inputQuery1.setLowerBound( 0, 100 );
        inputQuery1.setUpperBound( 0, 100 );
        tableau1.setLowerBound( 0, 100 );
        tableau1.setUpperBound( 0, 100 );

        // 0 <= x1 <= 2
        inputQuery1.setLowerBound( 1, 0 );
        inputQuery1.setUpperBound( 1, 2 );
        tableau1.setLowerBound( 1, 0 );
        tableau1.setUpperBound( 1, 2 );

        // 1 <= x2 <= 2
        inputQuery1.setLowerBound( 2, 1 );
        inputQuery1.setUpperBound( 2, 2 );
        tableau1.setLowerBound( 2, 1 );
        tableau1.setUpperBound( 2, 2 );

        // 3 <= x3 <= 4
        inputQuery1.setLowerBound( 3, 3 );
        inputQuery1.setUpperBound( 3, 4 );
        tableau1.setLowerBound( 3, 3 );
        tableau1.setUpperBound( 3, 4 );

        // For auxiliary vars of max constraint
        tableau1.setLowerBound( 4, 0 );
        tableau1.setUpperBound( 4, FloatUtils::infinity() );
        tableau1.setLowerBound( 5, 0 );
        tableau1.setUpperBound( 5, FloatUtils::infinity() );

        f = 1;
        MaxConstraint *max1 = new MaxConstraint( f, elements );
        max1->transformToUseAuxVariables( inputQuery1 );
        inputQuery1.addPiecewiseLinearConstraint( max1 );
        MILPEncoder milp1( tableau1 );
        milp1.encodeQuery( gurobi1, inputQuery1 );
        gurobi1.solve();

        TS_ASSERT( !gurobi1.haveFeasibleSolution() );

        //
        // x2 = max(x1, x2, x3)
        // Bounds:
        //   100 <= x0 <= 100
        //   0 <= x1 <= 2
        //   1 <= x2 <= 2
        //   3 <= x3 <= 4
        //
        GurobiWrapper gurobi2;

        Query inputQuery2 = Query();
        inputQuery2.setNumberOfVariables( 4 );

        MockTableau tableau2 = MockTableau();
        tableau2.setDimensions( 2, 6 );

        // 100 <= x0 <= 100
        inputQuery2.setLowerBound( 0, 100 );
        inputQuery2.setUpperBound( 0, 100 );
        tableau2.setLowerBound( 0, 100 );
        tableau2.setUpperBound( 0, 100 );

        // 0 <= x1 <= 2
        inputQuery2.setLowerBound( 1, 0 );
        inputQuery2.setUpperBound( 1, 2 );
        tableau2.setLowerBound( 1, 0 );
        tableau2.setUpperBound( 1, 2 );

        // 1 <= x2 <= 2
        inputQuery2.setLowerBound( 2, 1 );
        inputQuery2.setUpperBound( 2, 2 );
        tableau2.setLowerBound( 2, 1 );
        tableau2.setUpperBound( 2, 2 );

        // 3 <= x3 <= 4
        inputQuery2.setLowerBound( 3, 3 );
        inputQuery2.setUpperBound( 3, 4 );
        tableau2.setLowerBound( 3, 3 );
        tableau2.setUpperBound( 3, 4 );

        // For auxiliary vars of max constraint
        tableau2.setLowerBound( 4, 0 );
        tableau2.setUpperBound( 4, FloatUtils::infinity() );
        tableau2.setLowerBound( 5, 0 );
        tableau2.setUpperBound( 5, FloatUtils::infinity() );
        tableau2.setLowerBound( 6, 0 );
        tableau2.setUpperBound( 6, FloatUtils::infinity() );

        f = 2;
        MaxConstraint *max2 = new MaxConstraint( f, elements );
        max2->transformToUseAuxVariables( inputQuery2 );
        inputQuery2.addPiecewiseLinearConstraint( max2 );
        MILPEncoder milp2( tableau2 );
        milp2.encodeQuery( gurobi2, inputQuery2 );
        gurobi2.solve();

        TS_ASSERT( !gurobi2.haveFeasibleSolution() );

        //
        // x3 = max(x1, x2, x3)
        // Bounds:
        //   100 <= x0 <= 100
        //   0 <= x1 <= 2
        //   1 <= x2 <= 2
        //   3 <= x3 <= 4
        //
        GurobiWrapper gurobi3;

        Query inputQuery3 = Query();
        inputQuery3.setNumberOfVariables( 4 );

        MockTableau tableau3 = MockTableau();
        tableau3.setDimensions( 2, 6 );

        // 100 <= x0 <= 100
        inputQuery3.setLowerBound( 0, 100 );
        inputQuery3.setUpperBound( 0, 100 );
        tableau3.setLowerBound( 0, 100 );
        tableau3.setUpperBound( 0, 100 );

        // 0 <= x1 <= 2
        inputQuery3.setLowerBound( 1, 0 );
        inputQuery3.setUpperBound( 1, 2 );
        tableau3.setLowerBound( 1, 0 );
        tableau3.setUpperBound( 1, 2 );

        // 1 <= x2 <= 2
        inputQuery3.setLowerBound( 2, 1 );
        inputQuery3.setUpperBound( 2, 2 );
        tableau3.setLowerBound( 2, 1 );
        tableau3.setUpperBound( 2, 2 );

        // 3 <= x3 <= 4
        inputQuery3.setLowerBound( 3, 3 );
        inputQuery3.setUpperBound( 3, 4 );
        tableau3.setLowerBound( 3, 3 );
        tableau3.setUpperBound( 3, 4 );

        // For auxiliary vars of max constraint
        tableau3.setLowerBound( 4, 0 );
        tableau3.setUpperBound( 4, FloatUtils::infinity() );
        tableau3.setLowerBound( 5, 0 );
        tableau3.setUpperBound( 5, FloatUtils::infinity() );
        tableau3.setLowerBound( 6, 0 );
        tableau3.setUpperBound( 6, FloatUtils::infinity() );

        f = 3;
        MaxConstraint *max3 = new MaxConstraint( f, elements );
        max3->transformToUseAuxVariables( inputQuery3 );
        inputQuery3.addPiecewiseLinearConstraint( max3 );
        MILPEncoder milp3( tableau3 );
        milp3.encodeQuery( gurobi3, inputQuery3 );
        gurobi3.solve();

        TS_ASSERT( gurobi3.haveFeasibleSolution() );

        Map<String, double> values3;
        double costOrObjective3;

        gurobi3.extractSolution( values3, costOrObjective3 );

        double x0_sol3 = values3["x0"];
        double x1_sol3 = values3["x1"];
        double x2_sol3 = values3["x2"];
        double x3_sol3 = values3["x3"];

        TS_ASSERT_LESS_THAN_EQUALS( x1_sol3, x3_sol3 );
        TS_ASSERT_LESS_THAN_EQUALS( x2_sol3, x3_sol3 );
        TS_ASSERT_LESS_THAN_EQUALS( x3_sol3, x0_sol3 );
        TS_ASSERT_EQUALS( x0_sol3, 100.0 );

        //
        // x0 = max(x1, x2, x3)
        // Bounds:
        //   4 <= x0 <= 4
        //   0 <= x1 <= 2
        //   1 <= x2 <= 2
        //   3 <= x3 <= 4
        //
        GurobiWrapper gurobi4;

        Query inputQuery4 = Query();
        inputQuery4.setNumberOfVariables( 4 );

        MockTableau tableau4 = MockTableau();
        tableau4.setDimensions( 2, 6 );

        // 4 <= x0 <= 4
        inputQuery4.setLowerBound( 0, 4 );
        inputQuery4.setUpperBound( 0, 4 );
        tableau4.setLowerBound( 0, 4 );
        tableau4.setUpperBound( 0, 4 );

        // 0 <= x1 <= 2
        inputQuery4.setLowerBound( 1, 0 );
        inputQuery4.setUpperBound( 1, 2 );
        tableau4.setLowerBound( 1, 0 );
        tableau4.setUpperBound( 1, 2 );

        // 1 <= x2 <= 2
        inputQuery4.setLowerBound( 2, 1 );
        inputQuery4.setUpperBound( 2, 2 );
        tableau4.setLowerBound( 2, 1 );
        tableau4.setUpperBound( 2, 2 );

        // 3 <= x3 <= 4
        inputQuery4.setLowerBound( 3, 3 );
        inputQuery4.setUpperBound( 3, 4 );
        tableau4.setLowerBound( 3, 3 );
        tableau4.setUpperBound( 3, 4 );

        // For auxiliary vars of max constraint
        tableau4.setLowerBound( 4, 0 );
        tableau4.setUpperBound( 4, FloatUtils::infinity() );
        tableau4.setLowerBound( 5, 0 );
        tableau4.setUpperBound( 5, FloatUtils::infinity() );
        tableau4.setLowerBound( 6, 0 );
        tableau4.setUpperBound( 6, FloatUtils::infinity() );

        f = 0;
        MaxConstraint *max4 = new MaxConstraint( f, elements );
        max4->transformToUseAuxVariables( inputQuery4 );
        inputQuery4.addPiecewiseLinearConstraint( max4 );
        MILPEncoder milp4( tableau4 );
        milp4.encodeQuery( gurobi4, inputQuery4 );
        gurobi4.solve();

        TS_ASSERT( gurobi4.haveFeasibleSolution() );

        Map<String, double> values4;
        double costOrObjective4;

        gurobi4.extractSolution( values4, costOrObjective4 );

        double x0_sol4 = values4["x0"];
        double x1_sol4 = values4["x1"];
        double x2_sol4 = values4["x2"];
        double x3_sol4 = values4["x3"];

        TS_ASSERT_LESS_THAN_EQUALS( x1_sol4, x0_sol4 );
        TS_ASSERT_LESS_THAN_EQUALS( x2_sol4, x0_sol4 );
        TS_ASSERT_LESS_THAN_EQUALS( x3_sol4, x0_sol4 );
        TS_ASSERT_EQUALS( x0_sol4, 4.0 );

        //
        // x0 = max(x1, x2, x3)
        // Bounds:
        //   0 <= x0 <= 0
        //   0 <= x1 <= 2
        //   1 <= x2 <= 2
        //   3 <= x3 <= 4
        //
        GurobiWrapper gurobi5;

        Query inputQuery5 = Query();
        inputQuery5.setNumberOfVariables( 4 );

        MockTableau tableau5 = MockTableau();
        tableau5.setDimensions( 2, 6 );

        // 0 <= x0 <= 0
        inputQuery5.setLowerBound( 0, 0 );
        inputQuery5.setUpperBound( 0, 0 );
        tableau5.setLowerBound( 0, 0 );
        tableau5.setUpperBound( 0, 0 );

        // 0 <= x1 <= 2
        inputQuery5.setLowerBound( 1, 0 );
        inputQuery5.setUpperBound( 1, 2 );
        tableau5.setLowerBound( 1, 0 );
        tableau5.setUpperBound( 1, 2 );

        // 1 <= x2 <= 2
        inputQuery5.setLowerBound( 2, 1 );
        inputQuery5.setUpperBound( 2, 2 );
        tableau5.setLowerBound( 2, 1 );
        tableau5.setUpperBound( 2, 2 );

        // 3 <= x3 <= 4
        inputQuery5.setLowerBound( 3, 3 );
        inputQuery5.setUpperBound( 3, 4 );
        tableau5.setLowerBound( 3, 3 );
        tableau5.setUpperBound( 3, 4 );

        // For auxiliary vars of max constraint
        tableau5.setLowerBound( 4, 0 );
        tableau5.setUpperBound( 4, FloatUtils::infinity() );
        tableau5.setLowerBound( 5, 0 );
        tableau5.setUpperBound( 5, FloatUtils::infinity() );
        tableau5.setLowerBound( 6, 0 );
        tableau5.setUpperBound( 6, FloatUtils::infinity() );

        f = 0;
        MaxConstraint *max5 = new MaxConstraint( f, elements );
        max5->transformToUseAuxVariables( inputQuery5 );
        inputQuery5.addPiecewiseLinearConstraint( max5 );
        MILPEncoder milp5( tableau5 );
        milp5.encodeQuery( gurobi5, inputQuery5 );
        gurobi5.solve();

        TS_ASSERT( !gurobi5.haveFeasibleSolution() );

#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_eoncode_leaky_relu_constraint()
    {
#ifdef ENABLE_GUROBI

        //
        // x2 = leaky_relu(x0)
        // x3 = leaky_relu(x1)
        // x4 = x2 + x3
        // x4 = slope * -1 + 1

        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 5 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 1, 5 );

        // -1 <= x0 <= 1
        inputQuery1.setLowerBound( 0, -1 );
        inputQuery1.setUpperBound( 0, 1 );
        tableau1.setLowerBound( 0, -1 );
        tableau1.setUpperBound( 0, 1 );

        // -2 <= x1 <= -1
        inputQuery1.setLowerBound( 1, -2 );
        inputQuery1.setUpperBound( 1, -1 );
        tableau1.setLowerBound( 1, -2 );
        tableau1.setUpperBound( 1, -1 );

        //-1 <= x2 <= 1
        inputQuery1.setLowerBound( 2, -1 );
        inputQuery1.setUpperBound( 2, 1 );
        tableau1.setLowerBound( 2, -1 );
        tableau1.setUpperBound( 2, 1 );

        // -2 <= x3 <= 0
        inputQuery1.setLowerBound( 3, -2 );
        inputQuery1.setUpperBound( 3, 0 );
        tableau1.setLowerBound( 3, -2 );
        tableau1.setUpperBound( 3, 0 );

        // -3 <= x4 <= 1
        inputQuery1.setLowerBound( 4, -3 );
        inputQuery1.setUpperBound( 4, 1 );
        tableau1.setLowerBound( 4, -3 );
        tableau1.setUpperBound( 4, 1 );

        Equation eq;
        eq.addAddend( 1, 2 );
        eq.addAddend( 1, 3 );
        eq.addAddend( -1, 4 );
        inputQuery1.addEquation( eq );

        double slope = 0.2;
        LeakyReluConstraint *relu1 = new LeakyReluConstraint( 0, 2, slope );
        LeakyReluConstraint *relu2 = new LeakyReluConstraint( 1, 3, slope );
        inputQuery1.addPiecewiseLinearConstraint( relu1 );
        inputQuery1.addPiecewiseLinearConstraint( relu2 );

        MILPEncoder milp1( tableau1 );
        TS_ASSERT_THROWS_NOTHING( milp1.encodeQuery( gurobi1, inputQuery1 ) );
        TS_ASSERT_THROWS_NOTHING( gurobi1.solve() );
        TS_ASSERT( gurobi1.haveFeasibleSolution() );

        Query inputQuery2 = inputQuery1;

        Equation eq1;
        eq1.addAddend( 1, 4 );
        eq1.setScalar( slope * -1 + 1 );
        inputQuery2.addEquation( eq1 );

        GurobiWrapper gurobi2;
        MILPEncoder milp2( tableau1 );
        TS_ASSERT_THROWS_NOTHING( milp2.encodeQuery( gurobi2, inputQuery2 ) );
        TS_ASSERT_THROWS_NOTHING( gurobi2.solve() );
        TS_ASSERT( gurobi2.haveFeasibleSolution() );
        Map<String, double> values;
        double dontcare;
        TS_ASSERT_THROWS_NOTHING( gurobi2.extractSolution( values, dontcare ) );
        TS_ASSERT( FloatUtils::areEqual( values[Stringf( "x%u", 0 )], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( values[Stringf( "x%u", 1 )], -1 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_eoncode_leaky_relu_constraint_relax()
    {
#ifdef ENABLE_GUROBI

        //
        // x2 = leaky_relu(x0)
        // x3 = leaky_relu(x1)
        // x4 = x2 + x3
        // x4 = slope * -1 + 1

        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 5 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 1, 5 );

        // -1 <= x0 <= 1
        inputQuery1.setLowerBound( 0, -1 );
        inputQuery1.setUpperBound( 0, 1 );
        tableau1.setLowerBound( 0, -1 );
        tableau1.setUpperBound( 0, 1 );

        // -2 <= x1 <= -1
        inputQuery1.setLowerBound( 1, -2 );
        inputQuery1.setUpperBound( 1, -1 );
        tableau1.setLowerBound( 1, -2 );
        tableau1.setUpperBound( 1, -1 );

        //-1 <= x2 <= 1
        inputQuery1.setLowerBound( 2, -1 );
        inputQuery1.setUpperBound( 2, 1 );
        tableau1.setLowerBound( 2, -1 );
        tableau1.setUpperBound( 2, 1 );

        // -2 <= x3 <= 0
        inputQuery1.setLowerBound( 3, -2 );
        inputQuery1.setUpperBound( 3, 0 );
        tableau1.setLowerBound( 3, -2 );
        tableau1.setUpperBound( 3, 0 );

        // -3 <= x4 <= 1
        inputQuery1.setLowerBound( 4, -3 );
        inputQuery1.setUpperBound( 4, 1 );
        tableau1.setLowerBound( 4, -3 );
        tableau1.setUpperBound( 4, 1 );

        Equation eq;
        eq.addAddend( 1, 2 );
        eq.addAddend( 1, 3 );
        eq.addAddend( -1, 4 );
        inputQuery1.addEquation( eq );

        double slope = 0.2;
        LeakyReluConstraint *relu1 = new LeakyReluConstraint( 0, 2, slope );
        LeakyReluConstraint *relu2 = new LeakyReluConstraint( 1, 3, slope );
        inputQuery1.addPiecewiseLinearConstraint( relu1 );
        inputQuery1.addPiecewiseLinearConstraint( relu2 );

        MILPEncoder milp1( tableau1 );
        TS_ASSERT_THROWS_NOTHING( milp1.encodeQuery( gurobi1, inputQuery1, true ) );
        TS_ASSERT_THROWS_NOTHING( gurobi1.solve() );
        TS_ASSERT( gurobi1.haveFeasibleSolution() );

        Query inputQuery2 = inputQuery1;

        Equation eq1;
        eq1.addAddend( 1, 4 );
        eq1.setScalar( slope * -1 + 1 );
        inputQuery2.addEquation( eq1 );

        GurobiWrapper gurobi2;
        MILPEncoder milp2( tableau1 );
        TS_ASSERT_THROWS_NOTHING( milp2.encodeQuery( gurobi2, inputQuery2, true ) );
        TS_ASSERT_THROWS_NOTHING( gurobi2.solve() );
        TS_ASSERT( gurobi2.haveFeasibleSolution() );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_sigmoid_constraint_sat()
    {
#ifdef ENABLE_GUROBI

        /*
         * x0_lb >= 0
         */
        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 2 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 2, 2 );

        // 0 <= x0 <= 1
        inputQuery1.setLowerBound( 0, 0 );
        inputQuery1.setUpperBound( 0, 1 );
        tableau1.setLowerBound( 0, 0 );
        tableau1.setUpperBound( 0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid1 = new SigmoidConstraint( 0, 1 );
        inputQuery1.addNonlinearConstraint( sigmoid1 );
        inputQuery1.setLowerBound( 1, sigmoid1->sigmoid( 0 ) );
        inputQuery1.setUpperBound( 1, sigmoid1->sigmoid( 1 ) );
        tableau1.setLowerBound( 1, sigmoid1->sigmoid( 0 ) );
        tableau1.setUpperBound( 1, sigmoid1->sigmoid( 1 ) );

        MILPEncoder milp1( tableau1 );
        milp1.encodeQuery( gurobi1, inputQuery1 );

        TS_ASSERT_THROWS_NOTHING( gurobi1.solve() );

        TS_ASSERT( gurobi1.haveFeasibleSolution() );

        Map<String, double> solution1;
        double costValue1;

        TS_ASSERT_THROWS_NOTHING( gurobi1.extractSolution( solution1, costValue1 ) );

        TS_ASSERT( solution1.exists( "x0" ) );
        TS_ASSERT( solution1.exists( "x1" ) );
        TS_ASSERT( !solution1.exists( "a0" ) );

        /*
         * x0_ub < 0
         */
        GurobiWrapper gurobi2;

        Query inputQuery2 = Query();
        inputQuery2.setNumberOfVariables( 2 );

        MockTableau tableau2 = MockTableau();
        tableau2.setDimensions( 2, 2 );

        // -1 <= x0 < 0
        inputQuery2.setLowerBound( 0, -1 );
        inputQuery2.setUpperBound( 0, -0.1 );
        tableau2.setLowerBound( 0, -1 );
        tableau2.setUpperBound( 0, -0.1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid2 = new SigmoidConstraint( 0, 1 );
        inputQuery2.addNonlinearConstraint( sigmoid2 );
        inputQuery2.setLowerBound( 1, sigmoid2->sigmoid( -1 ) );
        inputQuery2.setUpperBound( 1, sigmoid2->sigmoid( -0.1 ) );
        tableau2.setLowerBound( 1, sigmoid2->sigmoid( -1 ) );
        tableau2.setUpperBound( 1, sigmoid2->sigmoid( -0.1 ) );

        MILPEncoder milp2( tableau2 );
        milp2.encodeQuery( gurobi2, inputQuery2 );

        TS_ASSERT_THROWS_NOTHING( gurobi2.solve() );

        TS_ASSERT( gurobi2.haveFeasibleSolution() );
        Map<String, double> solution2;
        double costValue2;

        TS_ASSERT_THROWS_NOTHING( gurobi2.extractSolution( solution2, costValue2 ) );

        TS_ASSERT( solution2.exists( "x0" ) );
        TS_ASSERT( solution2.exists( "x1" ) );
        TS_ASSERT( !solution2.exists( "a0" ) );

        /*
         * x0_lb < 0 and x0_ub > 0
         */
        GurobiWrapper gurobi3;

        Query inputQuery3 = Query();
        inputQuery3.setNumberOfVariables( 2 );

        MockTableau tableau3 = MockTableau();
        tableau3.setDimensions( 2, 2 );

        // -1 < x0 < 1
        inputQuery3.setLowerBound( 0, -1 );
        inputQuery3.setUpperBound( 0, 1 );
        tableau3.setLowerBound( 0, -1 );
        tableau3.setUpperBound( 0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid3 = new SigmoidConstraint( 0, 1 );
        inputQuery3.addNonlinearConstraint( sigmoid3 );
        inputQuery3.setLowerBound( 1, sigmoid3->sigmoid( -1 ) );
        inputQuery3.setUpperBound( 1, sigmoid3->sigmoid( 1 ) );
        tableau3.setLowerBound( 1, sigmoid3->sigmoid( -1 ) );
        tableau3.setUpperBound( 1, sigmoid3->sigmoid( 1 ) );

        MILPEncoder milp3( tableau3 );
        milp3.encodeQuery( gurobi3, inputQuery3 );

        TS_ASSERT_THROWS_NOTHING( gurobi3.solve() );

        TS_ASSERT( gurobi3.haveFeasibleSolution() );

        Map<String, double> solution3;
        double costValue3;

        TS_ASSERT_THROWS_NOTHING( gurobi3.extractSolution( solution3, costValue3 ) );

        TS_ASSERT( solution3.exists( "x0" ) );
        TS_ASSERT( solution3.exists( "x1" ) );
        TS_ASSERT( solution3.exists( "a0" ) );

        /*
         * x0_lb = 0 and x0_ub = 0
         */
        GurobiWrapper gurobi4;

        Query inputQuery4 = Query();
        inputQuery4.setNumberOfVariables( 2 );

        MockTableau tableau4 = MockTableau();
        tableau4.setDimensions( 2, 2 );

        // 0 <= x0 <= 0
        inputQuery4.setLowerBound( 0, 0 );
        inputQuery4.setUpperBound( 0, 0 );
        tableau4.setLowerBound( 0, 0 );
        tableau4.setUpperBound( 0, 0 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid4 = new SigmoidConstraint( 0, 1 );
        inputQuery4.addNonlinearConstraint( sigmoid4 );
        inputQuery4.setLowerBound( 1, sigmoid4->sigmoid( 0 ) );
        inputQuery4.setUpperBound( 1, sigmoid4->sigmoid( 0 ) );
        tableau4.setLowerBound( 1, sigmoid4->sigmoid( 0 ) );
        tableau4.setUpperBound( 1, sigmoid4->sigmoid( 0 ) );

        MILPEncoder milp4( tableau4 );
        milp4.encodeQuery( gurobi4, inputQuery4 );

        TS_ASSERT_THROWS_NOTHING( gurobi4.solve() );

        TS_ASSERT( gurobi4.haveFeasibleSolution() );

        Map<String, double> solution4;
        double costValue4;

        TS_ASSERT_THROWS_NOTHING( gurobi4.extractSolution( solution4, costValue4 ) );

        TS_ASSERT( solution4.exists( "x0" ) );
        TS_ASSERT( solution4.exists( "x1" ) );
        TS_ASSERT( !solution4.exists( "a0" ) );

        /*
         * x0_lb < 0 and x0_ub = 0
         */
        GurobiWrapper gurobi5;

        Query inputQuery5 = Query();
        inputQuery5.setNumberOfVariables( 2 );

        MockTableau tableau5 = MockTableau();
        tableau5.setDimensions( 2, 2 );

        // -1 <= x0 <= 0
        inputQuery5.setLowerBound( 0, -1 );
        inputQuery5.setUpperBound( 0, 0 );
        tableau5.setLowerBound( 0, -1 );
        tableau5.setUpperBound( 0, 0 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid5 = new SigmoidConstraint( 0, 1 );
        inputQuery5.addNonlinearConstraint( sigmoid5 );
        inputQuery5.setLowerBound( 1, sigmoid5->sigmoid( -1 ) );
        inputQuery5.setUpperBound( 1, sigmoid5->sigmoid( 0 ) );
        tableau5.setLowerBound( 1, sigmoid5->sigmoid( -1 ) );
        tableau5.setUpperBound( 1, sigmoid5->sigmoid( 0 ) );

        MILPEncoder milp5( tableau5 );
        milp5.encodeQuery( gurobi5, inputQuery5 );

        TS_ASSERT_THROWS_NOTHING( gurobi5.solve() );

        TS_ASSERT( gurobi5.haveFeasibleSolution() );

        Map<String, double> solution5;
        double costValue5;

        TS_ASSERT_THROWS_NOTHING( gurobi5.extractSolution( solution5, costValue5 ) );

        TS_ASSERT( solution5.exists( "x0" ) );
        TS_ASSERT( solution5.exists( "x1" ) );
        TS_ASSERT( !solution5.exists( "a0" ) );

        /*
         * x0_lb = 0 and x0_ub > 0
         */
        GurobiWrapper gurobi6;

        Query inputQuery6 = Query();
        inputQuery6.setNumberOfVariables( 2 );

        MockTableau tableau6 = MockTableau();
        tableau6.setDimensions( 2, 2 );

        // 0 <= x0 <= 1
        inputQuery6.setLowerBound( 0, 0 );
        inputQuery6.setUpperBound( 0, 1 );
        tableau6.setLowerBound( 0, 0 );
        tableau6.setUpperBound( 0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid6 = new SigmoidConstraint( 0, 1 );
        inputQuery6.addNonlinearConstraint( sigmoid6 );
        inputQuery6.setLowerBound( 1, sigmoid6->sigmoid( 0 ) );
        inputQuery6.setUpperBound( 1, sigmoid6->sigmoid( 1 ) );
        tableau6.setLowerBound( 1, sigmoid6->sigmoid( 0 ) );
        tableau6.setUpperBound( 1, sigmoid6->sigmoid( 1 ) );

        MILPEncoder milp6( tableau6 );
        milp6.encodeQuery( gurobi6, inputQuery6 );

        TS_ASSERT_THROWS_NOTHING( gurobi6.solve() );

        TS_ASSERT( gurobi6.haveFeasibleSolution() );

        Map<String, double> solution6;
        double costValue6;

        TS_ASSERT_THROWS_NOTHING( gurobi6.extractSolution( solution6, costValue6 ) );

        TS_ASSERT( solution6.exists( "x0" ) );
        TS_ASSERT( solution6.exists( "x1" ) );
        TS_ASSERT( !solution6.exists( "a0" ) );

#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_sigmoid_constraint_unsat()
    {
#ifdef ENABLE_GUROBI

        unsigned int x0 = 0U;
        unsigned int x1 = 1U;
        unsigned int x2 = 2U;

        /*
         * x0_lb >= 0
         */
        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 3 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 2, 3 );

        // 0 <= x0 <= 1
        inputQuery1.setLowerBound( x0, 0 );
        inputQuery1.setUpperBound( x0, 1 );
        tableau1.setLowerBound( x0, 0 );
        tableau1.setUpperBound( x0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid1 = new SigmoidConstraint( x0, x1 );
        inputQuery1.addNonlinearConstraint( sigmoid1 );
        inputQuery1.setLowerBound( x1, sigmoid1->sigmoid( 0 ) );
        inputQuery1.setUpperBound( x1, sigmoid1->sigmoid( 1 ) );
        tableau1.setLowerBound( x1, sigmoid1->sigmoid( 0 ) );
        tableau1.setUpperBound( x1, sigmoid1->sigmoid( 1 ) );

        // x2 = x1
        Equation equation1( Equation::EQ );
        equation1.addAddend( 1, x2 );
        equation1.addAddend( -1, x1 );
        equation1.setScalar( 0 );
        inputQuery1.addEquation( equation1 );
        inputQuery1.setLowerBound( x2, 0.5 * sigmoid1->sigmoid( 0 ) );
        inputQuery1.setUpperBound( x2, 0.5 * sigmoid1->sigmoid( 1 ) );
        tableau1.setLowerBound( x2, 0.5 * sigmoid1->sigmoid( 0 ) );
        tableau1.setUpperBound( x2, 0.5 * sigmoid1->sigmoid( 1 ) );

        MILPEncoder milp1( tableau1 );
        milp1.encodeQuery( gurobi1, inputQuery1 );

        TS_ASSERT_THROWS_NOTHING( gurobi1.solve() );

        TS_ASSERT( !gurobi1.haveFeasibleSolution() );

        /*
         * x0_ub < 0
         */
        GurobiWrapper gurobi2;

        Query inputQuery2 = Query();
        inputQuery2.setNumberOfVariables( 3 );

        MockTableau tableau2 = MockTableau();
        tableau2.setDimensions( 2, 3 );

        // -1 <= x0 < 0
        inputQuery2.setLowerBound( x0, -1 );
        inputQuery2.setUpperBound( x0, -0.1 );
        tableau2.setLowerBound( x0, -1 );
        tableau2.setUpperBound( x0, -0.1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid2 = new SigmoidConstraint( x0, x1 );
        inputQuery2.addNonlinearConstraint( sigmoid2 );
        inputQuery2.setLowerBound( x1, sigmoid2->sigmoid( -1 ) );
        inputQuery2.setUpperBound( x1, sigmoid2->sigmoid( -0.1 ) );
        tableau2.setLowerBound( x1, sigmoid2->sigmoid( -1 ) );
        tableau2.setUpperBound( x1, sigmoid2->sigmoid( -0.1 ) );

        // x2 = x1
        Equation equation2( Equation::EQ );
        equation2.addAddend( 1, x2 );
        equation2.addAddend( -1, x1 );
        equation2.setScalar( 0 );
        inputQuery2.addEquation( equation2 );
        inputQuery2.setLowerBound( x2, 0.5 * sigmoid2->sigmoid( -1 ) );
        inputQuery2.setUpperBound( x2, 0.5 * sigmoid2->sigmoid( -0.1 ) );
        tableau2.setLowerBound( x2, 0.5 * sigmoid2->sigmoid( -1 ) );
        tableau2.setUpperBound( x2, 0.5 * sigmoid2->sigmoid( -0.1 ) );

        MILPEncoder milp2( tableau2 );
        milp2.encodeQuery( gurobi2, inputQuery2 );

        TS_ASSERT_THROWS_NOTHING( gurobi2.solve() );

        TS_ASSERT( !gurobi2.haveFeasibleSolution() );

        /*
         * x0_lb < 0 and x0_ub > 0
         */
        GurobiWrapper gurobi3;

        Query inputQuery3 = Query();
        inputQuery3.setNumberOfVariables( 3 );

        MockTableau tableau3 = MockTableau();
        tableau3.setDimensions( 2, 3 );

        // -1 < x0 < 1
        inputQuery3.setLowerBound( x0, -1 );
        inputQuery3.setUpperBound( x0, 1 );
        tableau3.setLowerBound( x0, -1 );
        tableau3.setUpperBound( x0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid3 = new SigmoidConstraint( x0, x1 );
        inputQuery3.addNonlinearConstraint( sigmoid3 );
        inputQuery3.setLowerBound( x1, sigmoid3->sigmoid( -1 ) );
        inputQuery3.setUpperBound( x1, sigmoid3->sigmoid( 1 ) );
        tableau3.setLowerBound( x1, sigmoid3->sigmoid( -1 ) );
        tableau3.setUpperBound( x1, sigmoid3->sigmoid( 1 ) );

        // x2 = x1
        Equation equation3( Equation::EQ );
        equation3.addAddend( 1, x2 );
        equation3.addAddend( -1, x1 );
        equation3.setScalar( 0 );
        inputQuery3.addEquation( equation3 );
        inputQuery3.setLowerBound( x2, 0.5 * sigmoid3->sigmoid( -1 ) );
        inputQuery3.setUpperBound( x2, 0.5 * sigmoid3->sigmoid( 1 ) );
        tableau3.setLowerBound( x2, 0.5 * sigmoid3->sigmoid( -1 ) );
        tableau3.setUpperBound( x2, 0.5 * sigmoid3->sigmoid( -1 ) );

        MILPEncoder milp3( tableau3 );
        milp3.encodeQuery( gurobi3, inputQuery3 );

        TS_ASSERT_THROWS_NOTHING( gurobi3.solve() );

        TS_ASSERT( !gurobi3.haveFeasibleSolution() );

        /*
         * x0_lb = 0 and x0_ub = 0
         */
        GurobiWrapper gurobi4;

        Query inputQuery4 = Query();
        inputQuery4.setNumberOfVariables( 3 );

        MockTableau tableau4 = MockTableau();
        tableau4.setDimensions( 2, 3 );

        // 0 <= x0 <= 0
        inputQuery4.setLowerBound( x0, 0 );
        inputQuery4.setUpperBound( x0, 0 );
        tableau4.setLowerBound( x0, 0 );
        tableau4.setUpperBound( x0, 0 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid4 = new SigmoidConstraint( x0, x1 );
        inputQuery4.addNonlinearConstraint( sigmoid4 );
        inputQuery4.setLowerBound( x1, sigmoid4->sigmoid( 0 ) );
        inputQuery4.setUpperBound( x1, sigmoid4->sigmoid( 0 ) );
        tableau4.setLowerBound( x1, sigmoid4->sigmoid( 0 ) );
        tableau4.setUpperBound( x1, sigmoid4->sigmoid( 0 ) );

        // x2 = x1
        Equation equation4( Equation::EQ );
        equation4.addAddend( 1, x2 );
        equation4.addAddend( -1, x1 );
        equation4.setScalar( 0 );
        inputQuery4.addEquation( equation4 );
        inputQuery4.setLowerBound( x2, 0.5 * sigmoid4->sigmoid( 0 ) );
        inputQuery4.setUpperBound( x2, 0.5 * sigmoid4->sigmoid( 0 ) );
        tableau4.setLowerBound( x2, 0.5 * sigmoid4->sigmoid( 0 ) );
        tableau4.setUpperBound( x2, 0.5 * sigmoid4->sigmoid( 0 ) );

        MILPEncoder milp4( tableau4 );
        milp4.encodeQuery( gurobi4, inputQuery4 );

        TS_ASSERT_THROWS_NOTHING( gurobi4.solve() );

        TS_ASSERT( !gurobi4.haveFeasibleSolution() );

        /*
         * x0_lb < 0 and x0_ub = 0
         */
        GurobiWrapper gurobi5;

        Query inputQuery5 = Query();
        inputQuery5.setNumberOfVariables( 3 );

        MockTableau tableau5 = MockTableau();
        tableau5.setDimensions( 2, 3 );

        // -1 <= x0 <= 0
        inputQuery5.setLowerBound( x0, -1 );
        inputQuery5.setUpperBound( x0, 0 );
        tableau5.setLowerBound( x0, -1 );
        tableau5.setUpperBound( x0, 0 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid5 = new SigmoidConstraint( x0, x1 );
        inputQuery5.setLowerBound( x1, sigmoid5->sigmoid( -1 ) );
        inputQuery5.setUpperBound( x1, sigmoid5->sigmoid( 0 ) );
        tableau5.setLowerBound( x1, sigmoid5->sigmoid( -1 ) );
        tableau5.setUpperBound( x1, sigmoid5->sigmoid( 0 ) );

        // x2 = x1
        Equation equation5( Equation::EQ );
        equation5.addAddend( 1, x2 );
        equation5.addAddend( -1, x1 );
        equation5.setScalar( 0 );
        inputQuery5.addEquation( equation5 );
        inputQuery5.setLowerBound( x2, 0.5 * sigmoid5->sigmoid( -1 ) );
        inputQuery5.setUpperBound( x2, 0.5 * sigmoid5->sigmoid( 0 ) );
        tableau5.setLowerBound( x2, 0.5 * sigmoid5->sigmoid( -1 ) );
        tableau5.setUpperBound( x2, 0.5 * sigmoid5->sigmoid( 0 ) );

        inputQuery5.addNonlinearConstraint( sigmoid5 );
        MILPEncoder milp5( tableau5 );
        milp5.encodeQuery( gurobi5, inputQuery5 );

        TS_ASSERT_THROWS_NOTHING( gurobi5.solve() );

        TS_ASSERT( !gurobi5.haveFeasibleSolution() );

        /*
         * x0_lb = 0 and x0_ub > 0
         */
        GurobiWrapper gurobi6;

        Query inputQuery6 = Query();
        inputQuery6.setNumberOfVariables( 3 );

        MockTableau tableau6 = MockTableau();
        tableau6.setDimensions( 2, 3 );

        // 0 <= x0 <= 1
        inputQuery6.setLowerBound( x0, 0 );
        inputQuery6.setUpperBound( x0, 1 );
        tableau6.setLowerBound( x0, 0 );
        tableau6.setUpperBound( x0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid6 = new SigmoidConstraint( x0, x1 );
        inputQuery6.addNonlinearConstraint( sigmoid6 );
        inputQuery6.setLowerBound( x1, sigmoid6->sigmoid( 0 ) );
        inputQuery6.setUpperBound( x1, sigmoid6->sigmoid( 1 ) );
        tableau6.setLowerBound( x1, sigmoid6->sigmoid( 0 ) );
        tableau6.setUpperBound( x1, sigmoid6->sigmoid( 1 ) );

        // x2 = x1
        Equation equation6( Equation::EQ );
        equation6.addAddend( 1, x2 );
        equation6.addAddend( -1, x1 );
        equation6.setScalar( 0 );
        inputQuery6.addEquation( equation6 );
        inputQuery6.setLowerBound( x2, 0.5 * sigmoid6->sigmoid( 0 ) );
        inputQuery6.setUpperBound( x2, 0.5 * sigmoid6->sigmoid( 1 ) );
        tableau6.setLowerBound( x2, 0.5 * sigmoid6->sigmoid( 0 ) );
        tableau6.setUpperBound( x2, 0.5 * sigmoid6->sigmoid( 1 ) );

        MILPEncoder milp6( tableau6 );
        milp6.encodeQuery( gurobi6, inputQuery6 );

        TS_ASSERT_THROWS_NOTHING( gurobi6.solve() );

        TS_ASSERT( !gurobi6.haveFeasibleSolution() );

#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_bilinear_constraint1()
    {
#ifdef ENABLE_GUROBI

        /*
          1 <= x0 <= 0.5
          2 <= x1 <= 2
          -10 <= x2 <= 10
          -10 <= x3 <= 10
          x2 = x0 * x1
          x3 = x2 * x1
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 2, 2 );
        inputQuery.setLowerBound( 0, 0.5 );
        inputQuery.setUpperBound( 0, 0.5 );
        tableau.setLowerBound( 0, 0.5 );
        tableau.setUpperBound( 0, 0.5 );
        inputQuery.setLowerBound( 1, 2 );
        inputQuery.setUpperBound( 1, 2 );
        tableau.setLowerBound( 1, 2 );
        tableau.setUpperBound( 1, 2 );
        inputQuery.setLowerBound( 2, -10 );
        inputQuery.setUpperBound( 2, 10 );
        tableau.setLowerBound( 2, -10 );
        tableau.setUpperBound( 2, 10 );
        inputQuery.setLowerBound( 3, -10 );
        inputQuery.setUpperBound( 3, 10 );
        tableau.setLowerBound( 3, -10 );
        tableau.setUpperBound( 3, 10 );

        BilinearConstraint *bilinear1 = new BilinearConstraint( 0, 1, 2 );
        inputQuery.addNonlinearConstraint( bilinear1 );
        BilinearConstraint *bilinear2 = new BilinearConstraint( 2, 1, 3 );
        inputQuery.addNonlinearConstraint( bilinear2 );

        MILPEncoder milp( tableau );
        milp.encodeQuery( gurobi, inputQuery );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;

        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );

        TS_ASSERT( solution.exists( "x0" ) );
        TS_ASSERT( solution.exists( "x1" ) );
        TS_ASSERT( !solution.exists( "a0" ) );

        TS_ASSERT_EQUALS( solution["x0"], 0.5 );
        TS_ASSERT_EQUALS( solution["x1"], 2 );
        TS_ASSERT_EQUALS( solution["x2"], 1 );
        TS_ASSERT_EQUALS( solution["x3"], 2 );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_bilinear_constraint2()
    {
#ifdef ENABLE_GUROBI

        /*
          1 <= x0 <= 0.5
          2 <= x1 <= 2
          -10 <= x2 <= 0.5
          -10 <= x3 <= 10
          x2 = x0 * x1
          x3 = x2 * x1
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 2, 2 );
        inputQuery.setLowerBound( 0, 0.5 );
        inputQuery.setUpperBound( 0, 0.5 );
        tableau.setLowerBound( 0, 0.5 );
        tableau.setUpperBound( 0, 0.5 );
        inputQuery.setLowerBound( 1, 2 );
        inputQuery.setUpperBound( 1, 2 );
        tableau.setLowerBound( 1, 2 );
        tableau.setUpperBound( 1, 2 );
        inputQuery.setLowerBound( 2, -10 );
        inputQuery.setUpperBound( 2, 0.5 );
        tableau.setLowerBound( 2, -10 );
        tableau.setUpperBound( 2, 0.5 );
        inputQuery.setLowerBound( 3, -10 );
        inputQuery.setUpperBound( 3, 10 );
        tableau.setLowerBound( 3, -10 );
        tableau.setUpperBound( 3, 10 );

        BilinearConstraint *bilinear1 = new BilinearConstraint( 0, 1, 2 );
        inputQuery.addNonlinearConstraint( bilinear1 );
        BilinearConstraint *bilinear2 = new BilinearConstraint( 2, 1, 3 );
        inputQuery.addNonlinearConstraint( bilinear2 );

        MILPEncoder milp( tableau );
        milp.encodeQuery( gurobi, inputQuery );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( !gurobi.haveFeasibleSolution() );
        TS_ASSERT( gurobi.infeasible() );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_softmax_constraint()
    {
#ifdef ENABLE_GUROBI

        /*
          1.5 <= x0 <= 2
          0 <= x1 <= 0.5
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        SoftmaxConstraint *softmax = new SoftmaxConstraint( { 0, 1 }, { 2, 3 } );
        softmax->notifyLowerBound( 0, 1.5 );
        softmax->notifyUpperBound( 0, 2 );
        softmax->notifyLowerBound( 1, 0 );
        softmax->notifyUpperBound( 1, 0.5 );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 2, 2 );
        inputQuery.setLowerBound( 0, 1.5 );
        tableau.setLowerBound( 0, 1.5 );
        inputQuery.setUpperBound( 0, 2 );
        tableau.setUpperBound( 0, 2 );
        inputQuery.setLowerBound( 1, 0 );
        tableau.setLowerBound( 1, 0.5 );
        inputQuery.setUpperBound( 1, 0 );
        tableau.setUpperBound( 1, 0.5 );
        softmax->registerTableau( &tableau );

        List<Tightening> tightenings;
        softmax->getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
        {
            if ( t._type == Tightening::LB )
            {
                inputQuery.setLowerBound( t._variable, t._value );
                tableau.setLowerBound( t._variable, t._value );
            }
            if ( t._type == Tightening::UB )
            {
                inputQuery.setUpperBound( t._variable, t._value );
                tableau.setUpperBound( t._variable, t._value );
            }
        }
        inputQuery.addNonlinearConstraint( softmax );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_clip_constraint()
    {
#ifdef ENABLE_GUROBI

        /*
          1 <= x0 <= 4
          3 <= x1 <= 4
          x2 = x0 + x1
          x3 = Clip( x2, 2.5, 3)
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        Equation equation( Equation::EQ );
        equation.addAddend( 1, 0 );
        equation.addAddend( 1, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        double floor = 2.5;
        double ceiling = 3;
        TS_ASSERT_THROWS_NOTHING( inputQuery.addClipConstraint( 2, 3, floor, ceiling ) );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 5, 5 );
        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 4 );
        tableau.setLowerBound( 1, 3 );
        tableau.setUpperBound( 1, 4 );
        tableau.setLowerBound( 2, 4 );
        tableau.setUpperBound( 2, 8 );
        tableau.setLowerBound( 3, 0 );
        tableau.setUpperBound( 3, 8 );
        for ( unsigned var = 4; var < 8; ++var )
        {
            TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( var, 20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( var, 20 ) );
        }
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 7, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 7, 0 ) );

        for ( const auto &constraint : inputQuery.getPiecewiseLinearConstraints() )
            constraint->transformToUseAuxVariables( inputQuery );

        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 8, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 8, 100 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 9, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 9, 100 ) );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x3"], 3.0 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_clip_constraint2()
    {
#ifdef ENABLE_GUROBI

        /*
          -1 <= x0 <= 4
          -3 <= x1 <= 4
          x2 = x0 + x1
          x2 <= 2.5
          x3 = Clip( x2, 2.5, 3)
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        Equation equation( Equation::EQ );
        equation.addAddend( 1, 0 );
        equation.addAddend( 1, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        double floor = 2.5;
        double ceiling = 3;
        TS_ASSERT_THROWS_NOTHING( inputQuery.addClipConstraint( 2, 3, floor, ceiling ) );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 5, 5 );
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 4 );
        tableau.setLowerBound( 1, -3 );
        tableau.setUpperBound( 1, 4 );
        tableau.setLowerBound( 2, -4 );
        tableau.setUpperBound( 2, 2.5 );
        tableau.setLowerBound( 3, 0 );
        tableau.setUpperBound( 3, 8 );
        for ( unsigned var = 4; var < 8; ++var )
        {
            TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( var, 20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( var, 20 ) );
        }
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 7, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 7, 0 ) );

        for ( const auto &constraint : inputQuery.getPiecewiseLinearConstraints() )
            constraint->transformToUseAuxVariables( inputQuery );

        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 8, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 8, 100 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 9, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 9, 100 ) );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::lte( solution["x2"], 2.5 ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x3"], 2.5 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_clip_constraint3()
    {
#ifdef ENABLE_GUROBI

        /*
          1 <= x0 <= 4
          0 <= x1 <= 4
          x2 = x0 + x1
          2.5 <= x2 <= 3
          x3 = Clip( x2, 2.5, 3)
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        Equation equation( Equation::EQ );
        equation.addAddend( 1, 0 );
        equation.addAddend( 1, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        double floor = 2.5;
        double ceiling = 3;
        TS_ASSERT_THROWS_NOTHING( inputQuery.addClipConstraint( 2, 3, floor, ceiling ) );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 5, 5 );
        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 4 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 4 );
        tableau.setLowerBound( 2, 2.5 );
        tableau.setUpperBound( 2, 3 );
        tableau.setLowerBound( 3, 0 );
        tableau.setUpperBound( 3, 8 );
        for ( unsigned var = 4; var < 8; ++var )
        {
            TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( var, 20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( var, 20 ) );
        }
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 7, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 7, 0 ) );

        for ( const auto &constraint : inputQuery.getPiecewiseLinearConstraints() )
            constraint->transformToUseAuxVariables( inputQuery );

        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 8, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 8, 100 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 9, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 9, 100 ) );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x3"], solution["x2"] ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_clip_constraint4()
    {
#ifdef ENABLE_GUROBI

        /*
          1 <= x0 <= 4
          0 <= x1 <= 4
          x2 = x0 + x1
          x2 = 3
          x3 = Clip( x2, 2.5, 3)
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 4 );

        Equation equation( Equation::EQ );
        equation.addAddend( 1, 0 );
        equation.addAddend( 1, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        double floor = 2.5;
        double ceiling = 3;
        TS_ASSERT_THROWS_NOTHING( inputQuery.addClipConstraint( 2, 3, floor, ceiling ) );

        MockTableau tableau = MockTableau();
        tableau.setDimensions( 5, 5 );
        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 4 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 4 );
        tableau.setLowerBound( 2, 3 );
        tableau.setUpperBound( 2, 3 );
        tableau.setLowerBound( 3, 0 );
        tableau.setUpperBound( 3, 8 );
        for ( unsigned var = 4; var < 8; ++var )
        {
            TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( var, -20 ) );
            TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( var, 20 ) );
            TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( var, 20 ) );
        }
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 7, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 7, 0 ) );

        for ( const auto &constraint : inputQuery.getPiecewiseLinearConstraints() )
            constraint->transformToUseAuxVariables( inputQuery );

        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 8, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 8, 100 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setLowerBound( 9, 0 ) );
        TS_ASSERT_THROWS_NOTHING( tableau.setUpperBound( 9, 100 ) );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x2"], 3 ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x3"], 3 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_round_constraint1()
    {
#ifdef ENABLE_GUROBI

        /*
          1.2 <= x0 <= 1.3
          x1 = Round( x0 )
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 2 );

        RoundConstraint *round = new RoundConstraint( 0, 1 );

        round->notifyLowerBound( 0, 1.2 );
        round->notifyUpperBound( 0, 1.3 );
        MockTableau tableau = MockTableau();
        tableau.setDimensions( 1, 1 );
        inputQuery.setLowerBound( 0, 1.2 );
        tableau.setLowerBound( 0, 1.3 );

        List<Tightening> tightenings;
        round->getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
        {
            if ( t._type == Tightening::LB )
            {
                inputQuery.setLowerBound( t._variable, t._value );
                tableau.setLowerBound( t._variable, t._value );
            }
            if ( t._type == Tightening::UB )
            {
                inputQuery.setUpperBound( t._variable, t._value );
                tableau.setUpperBound( t._variable, t._value );
            }
        }
        round->registerTableau( &tableau );
        inputQuery.addNonlinearConstraint( round );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x1"], 1 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_round_constraint2()
    {
#ifdef ENABLE_GUROBI

        /*
          1.5 <= x0 <= 2.49999
          x1 = Round( x0 )
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 2 );

        RoundConstraint *round = new RoundConstraint( 0, 1 );

        round->notifyLowerBound( 0, 1.5 );
        round->notifyUpperBound( 0, 2.49999 );
        MockTableau tableau = MockTableau();
        tableau.setDimensions( 1, 1 );

        List<Tightening> tightenings;
        round->getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
        {
            if ( t._type == Tightening::LB )
            {
                inputQuery.setLowerBound( t._variable, t._value );
                tableau.setLowerBound( t._variable, t._value );
            }
            if ( t._type == Tightening::UB )
            {
                inputQuery.setUpperBound( t._variable, t._value );
                tableau.setUpperBound( t._variable, t._value );
            }
        }
        round->registerTableau( &tableau );
        inputQuery.addNonlinearConstraint( round );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x1"], 2 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_round_constraint3()
    {
#ifdef ENABLE_GUROBI

        /*
          1.6 <= x0 <= 1.7
          x1 = Round( x0 )
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 2 );

        RoundConstraint *round = new RoundConstraint( 0, 1 );

        round->notifyLowerBound( 0, 1.6 );
        round->notifyUpperBound( 0, 1.7 );
        MockTableau tableau = MockTableau();
        tableau.setDimensions( 1, 1 );
        inputQuery.setLowerBound( 0, 1.6 );
        tableau.setLowerBound( 0, 1.7 );

        List<Tightening> tightenings;
        round->getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
        {
            if ( t._type == Tightening::LB )
            {
                inputQuery.setLowerBound( t._variable, t._value );
                tableau.setLowerBound( t._variable, t._value );
            }
            if ( t._type == Tightening::UB )
            {
                inputQuery.setUpperBound( t._variable, t._value );
                tableau.setUpperBound( t._variable, t._value );
            }
        }
        round->registerTableau( &tableau );
        inputQuery.addNonlinearConstraint( round );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x1"], 2 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }

    void test_encode_round_constraint4()
    {
#ifdef ENABLE_GUROBI

        /*
          1.5 <= x0 <= 2.5
          x1 = Round( x0 )
        */
        GurobiWrapper gurobi;

        Query inputQuery = Query();
        inputQuery.setNumberOfVariables( 2 );

        RoundConstraint *round = new RoundConstraint( 0, 1 );

        round->notifyLowerBound( 0, 1.6 );
        round->notifyUpperBound( 0, 1.7 );
        MockTableau tableau = MockTableau();
        tableau.setDimensions( 1, 1 );
        inputQuery.setLowerBound( 0, 1.6 );
        tableau.setLowerBound( 0, 1.7 );
        inputQuery.setLowerBound( 1, 2 );
        tableau.setLowerBound( 1, 2 );

        List<Tightening> tightenings;
        round->getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
        {
            if ( t._type == Tightening::LB )
            {
                inputQuery.setLowerBound( t._variable, t._value );
                tableau.setLowerBound( t._variable, t._value );
            }
            if ( t._type == Tightening::UB )
            {
                inputQuery.setUpperBound( t._variable, t._value );
                tableau.setUpperBound( t._variable, t._value );
            }
        }
        round->registerTableau( &tableau );
        inputQuery.addNonlinearConstraint( round );

        MILPEncoder milp( tableau );
        TS_ASSERT_THROWS_NOTHING( milp.encodeQuery( gurobi, inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( gurobi.solve() );

        TS_ASSERT( gurobi.haveFeasibleSolution() );

        Map<String, double> solution;
        double costValue;
        // NOTE: the encoding for Round is sound but incomplete.
        TS_ASSERT_THROWS_NOTHING( gurobi.extractSolution( solution, costValue ) );
        TS_ASSERT( FloatUtils::areEqual( solution["x1"], 1 ) ||
                   FloatUtils::areEqual( solution["x1"], 2 ) ||
                   FloatUtils::areEqual( solution["x1"], 3 ) );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }
};
