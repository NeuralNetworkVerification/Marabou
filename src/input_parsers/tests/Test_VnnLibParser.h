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

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "InputQuery.h"
#include "OnnxParser.h"
#include "VnnLibParser.h"
#include <filesystem>

class VnnLibParserTestSuite : public CxxTest::TestSuite
{
public:
    void test_nano_vnncomp()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( -1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_tiny_vnncomp()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_tiny_vnncomp.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_tiny_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto *disjunction = ( DisjunctionConstraint * ) ( inputQuery.getPiecewiseLinearConstraints().back() );
        const auto &caseSplits = disjunction->getCaseSplits();
        const auto &caseSplitsIter = caseSplits.begin();
        auto eqIter = ( *caseSplitsIter ).getEquations().rbegin();

        Equation eq = *eqIter;
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( -100 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_small_vnncomp()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_small_vnncomp.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_small_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto *disjunction = ( DisjunctionConstraint * ) ( inputQuery.getPiecewiseLinearConstraints().back() );
        const auto &caseSplits = disjunction->getCaseSplits();
        const auto &caseSplitsIter = caseSplits.begin();
        auto eqIter = ( *caseSplitsIter ).getEquations().rbegin();

        Equation eq = *eqIter;
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( -100 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_sat_vnncomp()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_prop_vnncomp.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

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

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( -0.4933803235848431 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( 0.49999999998567607 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0.30353115613746867 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( -0.29855281193475053 );
        TS_ASSERT( eq.equivalent( testEq ) )

        Engine engine;
        engine.setVerbosity( 0 );
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        TS_ASSERT( engine.getExitCode() == Engine::ExitCode::SAT )
    }

    void test_unsat_vnncomp()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_prop_vnncomp.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_unsat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

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

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( -0.4933803235848431 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( 0.49999999998567607 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0.30353115613746867 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( -0.29855281193475053 );
        TS_ASSERT( eq.equivalent( testEq ) )

        Engine engine;
        engine.setVerbosity( 0 );
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        TS_ASSERT( engine.getExitCode() == Engine::ExitCode::UNSAT )
    }

    void test_add_const()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_add_const.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_add_var()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_add_var.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 2 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( -0.4933803235848431 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( 0.49999999998567607 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0.30353115613746867 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( -0.29855281193475053 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_sub_const()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sub_const.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( -2 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_sub_var()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sub_var.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_sat_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( 1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 1 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 2 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 1 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 4 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( -0.3 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 3 ) );
        testEq.setScalar( 0.5 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( -0.4933803235848431 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 2 ) );
        testEq.setScalar( 0.49999999998567607 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 1 ) );
        testEq.setScalar( 0.009549296585513092 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0.30353115613746867 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 1, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( -0.29855281193475053 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }

    void test_mul_var_const()
    {
        String filename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_mul_var_const.vnnlib" );
        String onnxFilename = Stringf( "%s/%s", RESOURCES_DIR "/onnx/vnnlib/", "test_nano_vnncomp.onnx" );

        InputQuery inputQuery;
        Equation testEq = Equation( Equation::EquationType::LE );

        OnnxParser onnxParser( onnxFilename );
        onnxParser.generateQuery( inputQuery );

        TS_ASSERT_THROWS_NOTHING( VnnLibParser().parse( filename, inputQuery ) );

        auto eqIter = inputQuery.getEquations().rbegin();

        Equation &eq = *eqIter;
        testEq.addAddend( -1, inputQuery.outputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 0, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 1000 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( 2, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 2 );
        TS_ASSERT( eq.equivalent( testEq ) )

        eqIter++;
        eq = *eqIter;
        testEq = Equation( Equation::EquationType::LE );
        testEq.addAddend( -2, inputQuery.inputVariableByIndex( 0 ) );
        testEq.setScalar( 0 );
        TS_ASSERT( eq.equivalent( testEq ) )
    }
};