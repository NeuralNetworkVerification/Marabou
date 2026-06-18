/*********************                                                        */
/*! \file Test_SmtLibWriter.h
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

#include "AletheProofWriter.h"
#include "CSRMatrix.h"
#include "MockFile.h"
#include "Query.h"
#include "context/cdlist.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>

using CVC4::context::Context;
using namespace CVC4::context;

class AletheProofWriterTestSuite : public CxxTest::TestSuite
{
public:
    MockFile *file;
    Context *context;

    void setUp()
    {
        TS_ASSERT_THROWS_NOTHING( file = new MockFile() );
        TS_ASSERT_THROWS_NOTHING( context = new Context );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete file; );
        TS_ASSERT_THROWS_NOTHING( delete context; );
    }

    /*
      Tests the writing Alethe assumption in correct SMTLIB format
    */
    void test_alethe_assumption_writing()
    {
        // Construct all required info.
        Vector<double> ubs = { 1, 1, 1, 0 };
        Vector<double> lbs = { 0, 0, 0, 0 };
        Vector<double> rows = { 1, 2, -1, 0, 1, -1, 1, 1 };

        unsigned n = ubs.size();
        unsigned m = 2;

        GroundBoundManager gbm = GroundBoundManager( *context );
        gbm.initialize( n );

        Query query = Query();
        query.setNumberOfVariables( n );

        ReluConstraint relu = ReluConstraint( 0, 1 );
        relu.transformToUseAuxVariables( query );
        relu.addTableauAuxVar( 3, 2 );
        List<PiecewiseLinearConstraint *> constraints = { &relu };

        CSRMatrix *matrix = new CSRMatrix();
        matrix->initialize( rows.data(), m, n );

        AletheProofWriter writer( m, ubs, lbs, gbm, matrix, constraints );
        writer.writeInstanceToFile( *file );

        String line;
        String expectedLine;

        // Tableau assumptions
        line = file->readLine( '\n' );
        expectedLine = "(assume e0(!(= 0.0 (+ x0 (* 2.0 x1) (- x2))):named e0))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume e1(!(= 0.0 (+ x0 (- x1) x2 x3)):named e1))";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Bound Assumptions
        line = file->readLine( '\n' );
        expectedLine = "(assume u0(!(<= x0 1.0):named u0))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume l0(!(>= x0 0.0):named l0))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume u1(!(<= x1 1.0):named u1))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume l1(!(>= x1 0.0):named l1))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume u2(!(<= x2 1.0):named u2))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume l2(!(>= x2 0.0):named l2))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume u3(!(<= x3 0.0):named u3))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "(assume l3(!(>= x3 0.0):named l3))";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Relu Assumptions
        line = file->readLine( '\n' );
        expectedLine = "(assume relu0 (ite (!(<= 0.0 x0):named a0)(= x0 x1)(<= x1 0.0)))";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        // Next lines should represent proof steps
        while ( line != "" )
        {
            expectedLine = "(step";
            TS_ASSERT_EQUALS( line.find( expectedLine ), 0U );
            line = file->readLine( '\n' );
        }

        delete matrix;
    }
};