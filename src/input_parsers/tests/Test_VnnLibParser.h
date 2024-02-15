/*********************                                                        */
/*! \file Test_VnnLibParser.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** Unit tests for the VnnLibParser class.
 **/

#include "Engine.h"
#include "InputQuery.h"
#include "OnnxParser.h"
#include "VnnLibParser.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>

class VnnLibParserTestSuite : public CxxTest::TestSuite
{
public:
    void test_nano_vnncomp()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        TS_ASSERT( lowerBounds.exists( inputVar ) && lowerBounds.get( inputVar ) == -1 )
        TS_ASSERT( upperBounds.exists( inputVar ) && upperBounds.get( inputVar ) == 1 )
        TS_ASSERT( upperBounds.exists( outputVar ) && upperBounds.get( outputVar ) == -1 )
    }

    void test_tiny_vnncomp()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_tiny_vnncomp.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_tiny_vnncomp.onnx" );

        InputQuery inputQuery;

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        auto *disjunction =
            (DisjunctionConstraint *)( inputQuery.getPiecewiseLinearConstraints().back() );
        const auto &caseSplits = disjunction->getCaseSplits();
        const auto &caseSplitsIter = caseSplits.begin();
        auto boundsIter = ( *caseSplitsIter ).getBoundTightenings().begin();

        Tightening t = *boundsIter;
        TS_ASSERT( t == Tightening( inputVar, -1, Tightening::LB ) )

        boundsIter++;
        t = *boundsIter;
        TS_ASSERT( t == Tightening( inputVar, 1, Tightening::UB ) )

        boundsIter++;
        t = *boundsIter;
        TS_ASSERT( t == Tightening( outputVar, 100, Tightening::LB ) )
    }

    void test_small_vnncomp()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_small_vnncomp.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_small_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        auto *disjunction =
            (DisjunctionConstraint *)( inputQuery.getPiecewiseLinearConstraints().back() );
        const auto &caseSplits = disjunction->getCaseSplits();
        const auto &caseSplitsIter = caseSplits.begin();
        auto boundsIter = ( *caseSplitsIter ).getBoundTightenings().begin();

        Tightening t = *boundsIter;
        TS_ASSERT( t == Tightening( inputVar, -1, Tightening::LB ) )

        boundsIter++;
        t = *boundsIter;
        TS_ASSERT( t == Tightening( inputVar, 1, Tightening::UB ) )

        boundsIter++;
        t = *boundsIter;
        TS_ASSERT( t == Tightening( outputVar, 100, Tightening::LB ) )
    }

    void test_sat_vnncomp()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_prop_vnncomp.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        unsigned int input0 = inputQuery.inputVariableByIndex( 0 );
        unsigned int input1 = inputQuery.inputVariableByIndex( 1 );
        unsigned int input2 = inputQuery.inputVariableByIndex( 2 );
        unsigned int input3 = inputQuery.inputVariableByIndex( 3 );
        unsigned int input4 = inputQuery.inputVariableByIndex( 4 );

        TS_ASSERT( lowerBounds.exists( input0 ) &&
                   lowerBounds.get( input0 ) == -0.30353115613746867 )
        TS_ASSERT( upperBounds.exists( input0 ) &&
                   upperBounds.get( input0 ) == -0.29855281193475053 )

        TS_ASSERT( lowerBounds.exists( input1 ) &&
                   lowerBounds.get( input1 ) == -0.009549296585513092 )
        TS_ASSERT( upperBounds.exists( input1 ) &&
                   upperBounds.get( input1 ) == 0.009549296585513092 )

        TS_ASSERT( lowerBounds.exists( input2 ) && lowerBounds.get( input2 ) == 0.4933803235848431 )
        TS_ASSERT( upperBounds.exists( input2 ) &&
                   upperBounds.get( input2 ) == 0.49999999998567607 )

        TS_ASSERT( lowerBounds.exists( input3 ) && lowerBounds.get( input3 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input3 ) && upperBounds.get( input3 ) == 0.5 )

        TS_ASSERT( lowerBounds.exists( input4 ) && lowerBounds.get( input4 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input4 ) && upperBounds.get( input4 ) == 0.5 )

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 4 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 3 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 2 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        Engine engine;
        engine.setVerbosity( 0 );
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        TS_ASSERT( engine.getExitCode() == Engine::ExitCode::SAT )
    }

    void test_unsat_vnncomp()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_prop_vnncomp.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_unsat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        unsigned int input0 = inputQuery.inputVariableByIndex( 0 );
        unsigned int input1 = inputQuery.inputVariableByIndex( 1 );
        unsigned int input2 = inputQuery.inputVariableByIndex( 2 );
        unsigned int input3 = inputQuery.inputVariableByIndex( 3 );
        unsigned int input4 = inputQuery.inputVariableByIndex( 4 );

        TS_ASSERT( lowerBounds.exists( input0 ) &&
                   lowerBounds.get( input0 ) == -0.30353115613746867 )
        TS_ASSERT( upperBounds.exists( input0 ) &&
                   upperBounds.get( input0 ) == -0.29855281193475053 )

        TS_ASSERT( lowerBounds.exists( input1 ) &&
                   lowerBounds.get( input1 ) == -0.009549296585513092 )
        TS_ASSERT( upperBounds.exists( input1 ) &&
                   upperBounds.get( input1 ) == 0.009549296585513092 )

        TS_ASSERT( lowerBounds.exists( input2 ) && lowerBounds.get( input2 ) == 0.4933803235848431 )
        TS_ASSERT( upperBounds.exists( input2 ) &&
                   upperBounds.get( input2 ) == 0.49999999998567607 )

        TS_ASSERT( lowerBounds.exists( input3 ) && lowerBounds.get( input3 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input3 ) && upperBounds.get( input3 ) == 0.5 )

        TS_ASSERT( lowerBounds.exists( input4 ) && lowerBounds.get( input4 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input4 ) && upperBounds.get( input4 ) == 0.5 )

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 4 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 3 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 2 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        Engine engine;
        engine.setVerbosity( 0 );
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        TS_ASSERT( engine.getExitCode() == Engine::ExitCode::UNSAT )
    }

    void test_add_const()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_add_const.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        TS_ASSERT( lowerBounds.exists( inputVar ) && lowerBounds.get( inputVar ) == 0 )
        TS_ASSERT( upperBounds.exists( inputVar ) && upperBounds.get( inputVar ) == 1 )
        TS_ASSERT( lowerBounds.exists( outputVar ) && lowerBounds.get( outputVar ) == 0 )
    }

    void test_add_var()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_add_var.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        unsigned int input0 = inputQuery.inputVariableByIndex( 0 );
        unsigned int input1 = inputQuery.inputVariableByIndex( 1 );
        unsigned int input2 = inputQuery.inputVariableByIndex( 2 );
        unsigned int input3 = inputQuery.inputVariableByIndex( 3 );
        unsigned int input4 = inputQuery.inputVariableByIndex( 4 );

        TS_ASSERT( lowerBounds.exists( input0 ) &&
                   lowerBounds.get( input0 ) == -0.30353115613746867 )
        TS_ASSERT( upperBounds.exists( input0 ) &&
                   upperBounds.get( input0 ) == -0.29855281193475053 )

        TS_ASSERT( lowerBounds.exists( input1 ) &&
                   lowerBounds.get( input1 ) == -0.009549296585513092 )
        TS_ASSERT( upperBounds.exists( input1 ) &&
                   upperBounds.get( input1 ) == 0.009549296585513092 )

        TS_ASSERT( lowerBounds.exists( input2 ) && lowerBounds.get( input2 ) == 0.4933803235848431 )
        TS_ASSERT( upperBounds.exists( input2 ) &&
                   upperBounds.get( input2 ) == 0.49999999998567607 )

        TS_ASSERT( lowerBounds.exists( input3 ) && lowerBounds.get( input3 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input3 ) && upperBounds.get( input3 ) == 0.5 )

        TS_ASSERT( lowerBounds.exists( input4 ) && lowerBounds.get( input4 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input4 ) && upperBounds.get( input4 ) == 0.5 )

        unsigned int output0 = inputQuery.outputVariableByIndex( 0 );
        unsigned int output1 = inputQuery.outputVariableByIndex( 1 );
        unsigned int output2 = inputQuery.outputVariableByIndex( 2 );

        TS_ASSERT( lowerBounds.exists( output0 ) && lowerBounds.get( output0 ) == 0 )
        TS_ASSERT( lowerBounds.exists( output1 ) && lowerBounds.get( output1 ) == 0 )
        TS_ASSERT( lowerBounds.exists( output2 ) && lowerBounds.get( output2 ) == 0 )

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, output0 );
        testEq.addAddend( 1, output1 );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_sub_const()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sub_const.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        TS_ASSERT( lowerBounds.exists( inputVar ) && lowerBounds.get( inputVar ) == 2 )
        TS_ASSERT( upperBounds.exists( inputVar ) && upperBounds.get( inputVar ) == 3 )
        TS_ASSERT( lowerBounds.exists( outputVar ) && lowerBounds.get( outputVar ) == 0 )
    }

    void test_sub_var()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sub_var.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        unsigned int input0 = inputQuery.inputVariableByIndex( 0 );
        unsigned int input1 = inputQuery.inputVariableByIndex( 1 );
        unsigned int input2 = inputQuery.inputVariableByIndex( 2 );
        unsigned int input3 = inputQuery.inputVariableByIndex( 3 );
        unsigned int input4 = inputQuery.inputVariableByIndex( 4 );

        TS_ASSERT( lowerBounds.exists( input0 ) &&
                   lowerBounds.get( input0 ) == -0.30353115613746867 )
        TS_ASSERT( upperBounds.exists( input0 ) &&
                   upperBounds.get( input0 ) == -0.29855281193475053 )

        TS_ASSERT( lowerBounds.exists( input1 ) &&
                   lowerBounds.get( input1 ) == -0.009549296585513092 )
        TS_ASSERT( upperBounds.exists( input1 ) &&
                   upperBounds.get( input1 ) == 0.009549296585513092 )

        TS_ASSERT( lowerBounds.exists( input2 ) && lowerBounds.get( input2 ) == 0.4933803235848431 )
        TS_ASSERT( upperBounds.exists( input2 ) &&
                   upperBounds.get( input2 ) == 0.49999999998567607 )

        TS_ASSERT( lowerBounds.exists( input3 ) && lowerBounds.get( input3 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input3 ) && upperBounds.get( input3 ) == 0.5 )

        TS_ASSERT( lowerBounds.exists( input4 ) && lowerBounds.get( input4 ) == 0.3 )
        TS_ASSERT( upperBounds.exists( input4 ) && upperBounds.get( input4 ) == 0.5 )

        unsigned int output0 = inputQuery.outputVariableByIndex( 0 );
        unsigned int output1 = inputQuery.outputVariableByIndex( 1 );
        unsigned int output2 = inputQuery.outputVariableByIndex( 2 );

        TS_ASSERT( lowerBounds.exists( output0 ) && lowerBounds.get( output0 ) == 0 )
        TS_ASSERT( lowerBounds.exists( output1 ) && lowerBounds.get( output1 ) == 0 )
        TS_ASSERT( lowerBounds.exists( output2 ) && lowerBounds.get( output2 ) == 0 )

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_mul_var_const()
    {
        String filename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_mul_var_const.vnnlib" );
        String onnxFilename =
            Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        unsigned int inputVar = inputQuery.inputVariableByIndex( 0 );
        unsigned int outputVar = inputQuery.outputVariableByIndex( 0 );

        const auto &lowerBounds = inputQuery.getLowerBounds();
        const auto &upperBounds = inputQuery.getUpperBounds();

        TS_ASSERT( lowerBounds.exists( inputVar ) && lowerBounds.get( inputVar ) == 0 )
        TS_ASSERT( upperBounds.exists( inputVar ) && upperBounds.get( inputVar ) == 1 )
        TS_ASSERT( lowerBounds.exists( outputVar ) && lowerBounds.get( outputVar ) == 0 )
    }
};