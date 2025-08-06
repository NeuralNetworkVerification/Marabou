//
// Created by maya-swisa on 8/6/25.
//

#ifndef TEST_ALPHACROWN_H
#define TEST_ALPHACROWN_H

#include "../../engine/tests/MockTableau.h"
#include "AcasParser.h"
#include "CWAttack.h"
#include "Engine.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "PropertyParser.h"
#include "Tightening.h"

#include <cxxtest/TestSuite.h>
#include <memory>

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
#ifdef BUILD_TORCH
        // todo set property to unast property (1.1) ?
        // todo create nlr
        auto networkFilePath =  "resources/nnet/acasxu/ACASXU_experimental_v2a_1_1.nnet";
        auto propertyFilePath =  String("resources/properties/acas_property_4.txt"); // todo check UNSAT
        // property
        auto *_acasParser = new AcasParser( networkFilePath );
        InputQuery _inputQuery;
        _acasParser->generateQuery( _inputQuery );
        PropertyParser().parse( propertyFilePath, _inputQuery );
        std::unique_ptr<Engine> _engine = std::make_unique<Engine>();
        _engine->processInputQuery( _inputQuery );
        NLR::NetworkLevelReasoner *_networkLevelReasoner = _engine->getNetworkLevelReasoner();
        TS_ASSERT_THROWS_NOTHING( _networkLevelReasoner->obtainCurrentBounds() );
        std::unique_ptr<CWAttack> cwAttack = std::make_unique<CWAttack>( _networkLevelReasoner );
        auto attackResultBeforeBoundTightening = cwAttack->runAttack();
        auto attackResultAfterBoundTightening = cwAttack->runAttack();
        TS_ASSERT( attackResultBeforeBoundTightening )
        TS_ASSERT_THROWS_NOTHING( _networkLevelReasoner->alphaCrownPropagation() );
        TS_ASSERT( !attackResultAfterBoundTightening )
#endif
    }

};

#endif // TEST_ALPHACROWN_H
