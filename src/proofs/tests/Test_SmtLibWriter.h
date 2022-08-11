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

#include "SmtLibWriter.h"
#include "context/cdlist.h"
#include "context/context.h"
#include <cxxtest/TestSuite.h>
#include "MockFile.h"

class SmtLibWriterTestSuite : public CxxTest::TestSuite
{
public:
    MockFile* file;

    /*
      Tests the whole functionality of the SmtLibWriter module
    */
    void testSmtLibWritting()
    {
        file = new MockFile();
        Vector<double> row = { 1, 1 };
        Vector<Vector<double>> initialTableau = { row };
        Vector<double> groundUpperBounds = { 1, 1 };
        Vector<double> groundLowerBounds = { 1 , -1 };
        List<String> instance;

        SmtLibWriter::addHeader( 2, instance );
        SmtLibWriter::addGroundUpperBounds( groundUpperBounds, instance );
        SmtLibWriter::addGroundLowerBounds( groundLowerBounds, instance );
        SmtLibWriter::addTableauRow( initialTableau[0], instance );
        SmtLibWriter::addReLUConstraint( 0, 1, PHASE_NOT_FIXED, instance );
        SmtLibWriter::addFooter( instance );

        SmtLibWriter::writeInstanceToFile( *file, instance );

        String line;
        String expectedLine;

        line = file->readLine( '\n' );
        expectedLine = "( set-logic QF_LRA )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( declare-fun x0 () Real )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( declare-fun x1 () Real )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( <= x0 1.000000 ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( <= x1 1.000000 ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( >= x0 1.000000 ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( >= x1 ( - 1.000000 ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( = 0 ( + ( * 1.000000 x0 ) ( * 1.000000 x1 ) ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( assert ( = x1 ( ite ( >= x0 0 ) x0 0 ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( check-sat )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( exit )";
        TS_ASSERT_EQUALS( line, expectedLine );
    }
};
