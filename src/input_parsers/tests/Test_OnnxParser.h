
#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "InputQuery.h"
#include "OnnxParser.h"

class OnnxParserTestSuite : public CxxTest::TestSuite
{
    public:
        const String NETWORK_FOLDER;

        OnnxParserTestSuite() : NETWORK_FOLDER("../../../resources/onnx/")
        {
        }

        void test_temp()
        {
            try
            {
                // Extract an input query from the network
                InputQuery inputQuery;

                OnnxParser* onnxParser = new OnnxParser( NETWORK_FOLDER + "/conv_mp1.onnx" );
                onnxParser->generateQuery( inputQuery );

                inputQuery.setLowerBound(0, 0);
                TS_ASSERT( 1 );
            }
            catch ( const Error &e )
            {
                printf( "Caught a %s error. Code: %u, Errno: %i, Message: %s.\n",
                        e.getErrorClass(),
                        e.getCode(),
                        e.getErrno(),
                        e.getUserMessage() );

                TS_ASSERT( 0 );
            }

        }
/**
    if ( argc != 2 )
    {
        printf( "Error: please provide an mps file\n");
        return 0;
    }

    char *filename = argv[1];


    MpsParser mpsParser( filename );
    mpsParser.generateQuery( inputQuery );

    Engine engine;
    bool preprocess = engine.processInputQuery( inputQuery );

    if ( !preprocess || !engine.solve() )
    {
        printf( "unsat\n" );
        return 0;
    }

    printf( "sat\n" );

    // Uncomment the below to print the solution


    return 0;
    **/
};