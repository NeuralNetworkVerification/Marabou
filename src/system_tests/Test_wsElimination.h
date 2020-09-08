/*********************                                                        */
/*! \file Test_wsElimination.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/




#ifndef MARABOU_TEST_WSELIMINATION_H
#define MARABOU_TEST_WSELIMINATION_H



#include <cxxtest/TestSuite.h>

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "QueryLoader.h"
#include "MarabouError.h"

class wsEliminationTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_nlr_to_query_and_back()
    {

        Engine engine;
        InputQuery inputQuery = QueryLoader::loadQuery (RESOURCES_DIR "/bnn_queries/smallBNN_original");
        for (unsigned inputVariable = 0; inputVariable  < 784; ++ inputVariable)
        {
            auto pixel = inputVariable / 784 ;

            inputQuery.setLowerBound(inputVariable, pixel);
            inputQuery.setUpperBound(inputVariable, pixel);
        }
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( engine.solve() ); // todo fix

        engine.extractSolution( inputQuery );
        inputQuery.getSolutionValue(0); // todo fix for output variables

    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//



#endif //MARABOU_TEST_WSELIMINATION_H
