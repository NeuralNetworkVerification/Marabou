/*********************                                                        */
/*! \file Test_Disjunction.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** Unit tests for the OnnxParser class.
 **/

#include "Engine.h"
#include "InputQuery.h"
#include "OnnxParser.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>

class OnnxParserTestSuite : public CxxTest::TestSuite
{
public:
    const double DELTA = 0.0001;

    void expect_error( String name )
    {
        // Extract an input query from the network

        String networkPath = Stringf( "%s/%s.onnx", RESOURCES_DIR "/onnx/layer-zoo", name.ascii() );
        if ( !File::exists( networkPath ) )
        {
            printf( "Error: the specified test inputQuery file (%s) doesn't exist!\n",
                    networkPath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkPath.ascii() );
        }

        InputQueryBuilder queryBuilder;
        TS_ASSERT_THROWS_EQUALS( OnnxParser::parse( queryBuilder, networkPath, {}, {} ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::ONNX_PARSER_ERROR );
    }

    void run_test( String name, Vector<double> inputValues, Vector<double> expectedOutputValues )
    {
        // Extract an input query from the network

        String networkPath = Stringf( "%s/%s.onnx", RESOURCES_DIR "/onnx/layer-zoo", name.ascii() );
        if ( !File::exists( networkPath ) )
        {
            printf( "Error: the specified test inputQuery file (%s) doesn't exist!\n",
                    networkPath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkPath.ascii() );
        }

        InputQueryBuilder queryBuilder;
        TS_ASSERT_THROWS_NOTHING( OnnxParser::parse( queryBuilder, networkPath, {}, {} ) );

        InputQuery inputQuery;
        queryBuilder.generateQuery( inputQuery );

        TS_ASSERT( inputValues.size() == inputQuery.getNumInputVariables() );
        TS_ASSERT( expectedOutputValues.size() == inputQuery.getNumOutputVariables() );

        for ( unsigned int i = 0; i < inputValues.size(); i++ )
        {
            unsigned int inputVariable = inputQuery.inputVariableByIndex( i );
            inputQuery.setLowerBound( inputVariable, inputValues[i] );
            inputQuery.setUpperBound( inputVariable, inputValues[i] );
        }

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        engine.extractSolution( inputQuery );

        for ( unsigned int i = 0; i < expectedOutputValues.size(); ++i )
        {
            unsigned int outputVariable = inputQuery.outputVariableByIndex( i );
            double actualOutputValue = inputQuery.getSolutionValue( outputVariable );
            double expectedOutputValue = expectedOutputValues[i];

            TS_ASSERT_DELTA( actualOutputValue, expectedOutputValue, DELTA );
        }
    }

    // Disabled due to https://github.com/NeuralNetworkVerification/Marabou/issues/637
    // Constant nodes are tested anyway as part of the other networks which require
    // constant inputs.
    //
    // void test_constant()
    // {
    //     Vector<double> input = {
    //         0.5
    //     };
    //     Vector<double> output = {
    //         0, 0.5,
    //         1, 1.5
    //     };
    //     run_test("constant", input, output);
    // }

    void test_identity()
    {
        Vector<double> input = {
            0,
            0.5, //
            1,
            1.5, //
        };
        Vector<double> output = {
            0,
            0.5, //
            1,
            1.5, //
        };
        run_test( "identity", input, output );
    }

    void test_reshape()
    {
        Vector<double> input = {
            0,
            0.5, //
            1,
            1.5, //
        };
        Vector<double> output = {
            0,   //
            0.5, //
            1,   //
            1.5, //
        };
        run_test( "reshape", input, output );
    }

    void test_reshape_with_dimension_inference()
    {
        Vector<double> input = {
            0,
            0.5, //
            1,
            1.5 //
        };
        Vector<double> output = {
            0,   //
            0.5, //
            1,   //
            1.5  //
        };
        run_test( "reshape_with_dimension_inference", input, output );
    }

    void test_flatten()
    {
        Vector<double> input = {
            0,  0.5, //
            1,  1.5, //

            -1, -3,  //
            -4, 0.0, //
        };
        Vector<double> output = {
            0,  0.5, 1,  1.5, //
            -1, -3,  -4, 0.0, //
        };
        run_test( "flatten", input, output );
    }

    void test_transpose()
    {
        Vector<double> input = {
            0,   1,   -2, //
            0.5, 1.5, -3, //
        };
        Vector<double> output = {
            0,  0.5, //
            1,  1.5, //
            -2, -3,  //
        };
        run_test( "transpose", input, output );
    }

    void test_squeeze()
    {
        Vector<double> input = {
            0,   1,   -2, //
            0.5, 1.5, -3, //
        };
        Vector<double> output = {
            0,   1,   -2, //
            0.5, 1.5, -3, //
        };
        run_test( "squeeze", input, output );
    }

    void test_squeeze_with_axes()
    {
        Vector<double> input = {
            0,   1,   -2, //
            0.5, 1.5, -3, //
        };
        Vector<double> output = {
            0,   1,   -2, //
            0.5, 1.5, -3, //
        };
        run_test( "squeeze_with_axes", input, output );
    }

    void test_unsqueeze()
    {
        Vector<double> input = { 0, 1, -2, 0.5, 1.5, -3 };
        Vector<double> output = { 0, 1, -2, 0.5, 1.5, -3 };
        run_test( "unsqueeze", input, output );
    }

    // Reference implementation for batch normalisation
    double batchnorm_fn( double input,
                         double epsilon,
                         double scale,
                         double bias,
                         double mean,
                         double variance )
    {
        return ( input - mean ) / sqrt( variance + epsilon ) * scale + bias;
    }

    void test_batch_normalization()
    {
        double eps = 1e-05;
        Vector<double> input = {
            0, 1, //
            2, 3, //
            4, 5, //
        };
        Vector<double> output = {
            batchnorm_fn( 0, eps, 0.5, 0, 5, 0.5 ), batchnorm_fn( 1, eps, 0.5, 0, 5, 0.5 ),
            batchnorm_fn( 2, eps, 1, 1, 6, 0.5 ),   batchnorm_fn( 3, eps, 1, 1, 6, 0.5 ),
            batchnorm_fn( 4, eps, 2, 0, 7, 0.5 ),   batchnorm_fn( 5, eps, 2, 0, 7, 0.5 ),
        };
        run_test( "batchnorm", input, output );
    }

    void test_maxpool()
    {
        // Taken from https://github.com/onnx/onnx/blob/main/docs/Operators.md#maxpool
        Vector<double> input = {
            1,  2,  3,  4,  //
            5,  6,  7,  8,  //
            9,  10, 11, 12, //
            13, 14, 15, 16, //
        };
        Vector<double> output = {
            11,
            12, //
            15,
            16, //
        };
        run_test( "maxpool", input, output );
    }

    void test_conv()
    {
        // Taken from https://github.com/onnx/onnx/blob/main/docs/Operators.md#Conv
        Vector<double> input = {
            0.0,  1.0,  2.0,  3.0,  4.0,  //
            5.0,  6.0,  7.0,  8.0,  9.0,  //
            10.0, 11.0, 12.0, 13.0, 14.0, //
            15.0, 16.0, 17.0, 18.0, 19.0, //
            20.0, 21.0, 22.0, 23.0, 24.0, //
        };
        Vector<double> output = {
            12.0, 21.0,  27.0,  33.0,  24.0,  //
            33.0, 54.0,  63.0,  72.0,  51.0,  //
            63.0, 99.0,  108.0, 117.0, 81.0,  //
            93.0, 144.0, 153.0, 162.0, 111.0, //
            72.0, 111.0, 117.0, 123.0, 84.0,  //
        };
        run_test( "conv", input, output );
    }

    void test_gemm()
    {
        // 0.25 * input * [[0.5, 1.0], [1.0, 2.0]]^T  + 0.5 * [3, 4.5]
        Vector<double> input = {
            0.0,
            1.0, //
            2.0,
            3.0, //
        };
        Vector<double> output = {
            1.75,
            2.75, //
            2.5,
            4.5, //
        };
        run_test( "gemm", input, output );
    }

    void test_relu()
    {
        Vector<double> input = {
            -2.0,
            0.0, //
            0.1,
            3.0, //
        };
        Vector<double> output = {
            0,
            0, //
            0.1,
            3, //
        };
        run_test( "relu", input, output );
    }

    void test_leakyRelu()
    {
        Vector<double> input = {
            -2.0,
            0.0, //
            0.1,
            3.0, //
        };
        Vector<double> output = {
            -0.1,
            0, //
            0.1,
            3, //
        };
        run_test( "leakyRelu", input, output );
    }

    void test_add()
    {
        Vector<double> input = {
            -2.0,
            0.0, //
            0.1,
            3.0, //
        };
        Vector<double> output = {
            -1.5,
            1, //
            1.6,
            5, //
        };
        run_test( "add", input, output );
    }

    void test_sub()
    {
        Vector<double> input = {
            -2.0,
            0.0, //
            0.1,
            3.0, //
        };
        Vector<double> output = {
            -2.5,
            -1, //
            -1.4,
            1, //
        };
        run_test( "sub", input, output );
    }

    void test_matmul()
    {
        Vector<double> input = {
            -2,  0, 1, //
            1.5, 2, 3, //
        };
        Vector<double> output = {
            -1,
            -3, //
            0,
            -1.25, //
        };
        run_test( "matmul", input, output );
    }

    double sigmoid_fn( double value )
    {
        return 1.0 / ( 1.0 + exp( -value ) );
    }

    void test_sigmoid()
    {
        Vector<double> input = { -2, 0, 1, 3.5 };
        Vector<double> output = {
            sigmoid_fn( -2 ), sigmoid_fn( 0 ), sigmoid_fn( 1 ), sigmoid_fn( 3.5 )
        };
        run_test( "sigmoid", input, output );
    }

    double tanh_fn( double value )
    {
        return 2 * sigmoid_fn( 2 * value ) - 1;
    }

    void test_tanh()
    {
        Vector<double> input = { -2, 0, 1, 3.5 };
        Vector<double> output = { tanh_fn( -2 ), tanh_fn( 0 ), tanh_fn( 1 ), tanh_fn( 3.5 ) };
        run_test( "tanh", input, output );
    }

    void test_cast_int_to_float()
    {
        Vector<double> input = {
            0.0,
            0.0, //
            0.0,
            0.0, //
        };
        Vector<double> output = {
            0.0,
            1.0, //
            -1.0,
            2.0, //
        };
        run_test( "cast_int_to_float", input, output );
    }

    void test_dropout()
    {
        Vector<double> input = { 1, 2, 1.5, -1 };
        Vector<double> output = { 1, 2, 1.5, -1 };
        run_test( "dropout", input, output );
    }

    void test_dropout_training_mode_false()
    {
        Vector<double> input = { 1, 2, 1.5, -1 };
        Vector<double> output = { 1, 2, 1.5, -1 };
        run_test( "dropout_training_mode_false", input, output );
    }

    void test_dropout_training_mode_true()
    {
        expect_error( "dropout_training_mode_true" );
    }
};
