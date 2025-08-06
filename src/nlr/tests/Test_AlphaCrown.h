//
// Created by maya-swisa on 8/6/25.
//

#ifndef TEST_ALPHACROWN_H
#define TEST_ALPHACROWN_H

#include "../../engine/tests/MockTableau.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "Tightening.h"
#include "AcasParser.h"

#include <cxxtest/TestSuite.h>

class AlphaCrownAnalysisTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void testWithAttack()
    {
        // todo set property to unast property (1.1) ?
        // todo create nlr
        auto networkFilePath = "resources/onnx/acasxu/ACASXU_experimental_v2a_1_1.onnx";
        auto propertyFilePath = "resources/properties/acas_property_4.txt"; // todo check UNSAT property
        AcasParser *_acasParser = new AcasParser( networkFilePath );
        InputQuery _inputQuery;
        _acasParser->generateQuery( _inputQuery );
        PropertyParser().parse( propertyFilePath, _inputQuery );
        CWAttack cwAttack = std::make_unique<CWAttack>( _networkLevelReasoner );
        auto attackResultBeforeBoundTightening =     cwAttack->runAttack();
        // todo call alpha crown
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.alphaCrownPropagation() );
        TS_ASSERT( !_cwAttack->runAttack() )
// todo maybe all it loop

}
};

#endif //TEST_ALPHACROWN_H
