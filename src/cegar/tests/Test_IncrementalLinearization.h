/*********************                                                        */
/*! \file Test_IncrementalLinearization.h
** \verbatim
** Top contributors (to current version):
**   Teruhiro Tagomori, Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]
**/

#include "Engine.h"
#include "FloatUtils.h"
#include "IncrementalLinearization.h"
#include "InputQuery.h"
#include "Options.h"
#include "SigmoidConstraint.h"

#include <cxxtest/TestSuite.h>
#include <string.h>


using namespace CEGAR;

class IncrementalLinearizationTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void run_sigmoid_test( unsigned testNumber, double lb, double ub, double bValue, double fValue )
    {
        std::cout << "Sigmoid case " << testNumber << std::endl;
        InputQuery ipq;
        ipq.setNumberOfVariables( 2 );
        ipq.setLowerBound( 0, lb );
        ipq.setUpperBound( 0, ub );
        ipq.addNonlinearConstraint( new SigmoidConstraint( 0, 1 ) );
        Engine *dummy = new Engine();
        IncrementalLinearization cegarEngine( ipq, dummy );

        Query refinement;
        refinement.setNumberOfVariables( 2 );
        refinement.setLowerBound( 0, lb );
        refinement.setUpperBound( 0, ub );
        refinement.setSolutionValue( 0, bValue );
        refinement.setSolutionValue( 1, fValue );
        TS_ASSERT_THROWS_NOTHING( cegarEngine.refine( refinement ) );

        Engine engine;
        engine.setVerbosity( 2 );
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( ipq ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );
        Engine::ExitCode code = engine.getExitCode();
        TS_ASSERT( code == Engine::SAT || code == Engine::UNKNOWN );
    }

    void _test_incremental_linearization_sigmoid()
    {
        // Case 1
        run_sigmoid_test( 1, -10, 10, -3, 0.00001 );
        return;
        // Case 2
        run_sigmoid_test( 2, -10, 10, 0, 0.01 );

        // Case 3
        run_sigmoid_test( 3, -10, 10, 3, 0.01 );

        // Case 4
        run_sigmoid_test( 4, 1, 10, 3, 0.5 );

        // Case 5
        run_sigmoid_test( 5, -10, -2, -3, 0.00001 );


        // Case 6
        run_sigmoid_test( 6, -10, 10, -3, 0.08 );

        // Case 7
        run_sigmoid_test( 7, -10, 10, 0, 0.8 );

        // Case 8
        run_sigmoid_test( 8, -10, 10, 3, 0.999 );

        // Case 9
        run_sigmoid_test( 9, 1, 10, 3, 0.999 );

        // Case 10
        run_sigmoid_test( 10, -10, -2, -3, 0.08 );
    }

    void run_inc_lin_cegar_test( List<Equation> &additionalConstraints )
    {
        unsigned b = 0;
        unsigned f = 1;
        InputQuery ipq;
        ipq.setNumberOfVariables( 2 );
        ipq.setLowerBound( 0, -10 );
        ipq.setUpperBound( 0, 10 );
        ipq.addNonlinearConstraint( new SigmoidConstraint( b, f ) );

        for ( const auto &e : additionalConstraints )
            ipq.addEquation( e );

        Options::get()->setString( Options::LP_SOLVER, "native" );

        Engine *initialEngine = new Engine();
        initialEngine->setVerbosity( 0 );
        TS_ASSERT( initialEngine->processInputQuery( ipq ) );
        TS_ASSERT_THROWS_NOTHING( initialEngine->solve() );

        TS_ASSERT( initialEngine->getExitCode() == Engine::UNKNOWN );
        TS_ASSERT( initialEngine->getExitCode() == Engine::UNKNOWN ||
                   initialEngine->getExitCode() == Engine::SAT );
        if ( initialEngine->getExitCode() == Engine::SAT )
        {
            delete initialEngine;
            return;
        }
        initialEngine->extractSolution( ipq );
        IncrementalLinearization cegarEngine( ipq, initialEngine );
        TS_ASSERT_THROWS_NOTHING( cegarEngine.solve() );
        Engine *afterEngine = nullptr;
        TS_ASSERT_THROWS_NOTHING( afterEngine = cegarEngine.releaseEngine() );

        afterEngine->extractSolution( ipq );
        std::cout << ipq.getSolutionValue( 0 ) << std::endl;
        std::cout << ipq.getSolutionValue( 1 ) << std::endl;

        TS_ASSERT_EQUALS( afterEngine->getExitCode(), Engine::SAT );

        if ( afterEngine )
            delete afterEngine;
    }

    void test_incremental_linearization_sigmoid_sat()
    {
        unsigned b = 0;
        unsigned f = 1;
        {
            List<Equation> equations;
            {
                Equation e;
                e.setType( Equation::GE );
                e.addAddend( 1, f );
                e.addAddend( -1, b );
                e.setScalar( 0 );
                equations.append( e );
            }
            run_inc_lin_cegar_test( equations );
        }

        {
            List<Equation> equations;
            {
                Equation e;
                e.setType( Equation::GE );
                e.addAddend( 1, f );
                e.addAddend( -1, b );
                e.setScalar( 0 );
                equations.append( e );
            }

            {
                Equation e;
                e.setType( Equation::GE );
                e.addAddend( 1, f );
                e.addAddend( 1, b );
                e.setScalar( 0 );
                equations.append( e );
            }
            run_inc_lin_cegar_test( equations );
        }
    }
};
