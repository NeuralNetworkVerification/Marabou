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

#include "MockFile.h"
#include "SmtLibWriter.h"
#include "context/cdlist.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>

class SmtLibWriterTestSuite : public CxxTest::TestSuite
{
public:
    MockFile *file;

    /*
      Tests the whole functionality of the SmtLibWriter module
    */
    void test_smtlib_writing()
    {
        file = new MockFile();
        Vector<double> row = { 1, 2 };
        SparseUnsortedList sparseRow( row.data(), 2 );
        Vector<double> groundUpperBounds = { 1, 1 };
        Vector<double> groundLowerBounds = { 1, -1 };
        List<String> instance;

        SmtLibWriter::addHeader( 2, instance );
        SmtLibWriter::addGroundUpperBounds( groundUpperBounds, instance );
        SmtLibWriter::addGroundLowerBounds( groundLowerBounds, instance );
        SmtLibWriter::addTableauRow( sparseRow, instance );
        SmtLibWriter::addReLUConstraint( 0, 1, PHASE_NOT_FIXED, instance );
        SmtLibWriter::addSignConstraint( 0, 1, PHASE_NOT_FIXED, instance );
        SmtLibWriter::addAbsConstraint( 0, 1, PHASE_NOT_FIXED, instance );
        SmtLibWriter::addLeakyReLUConstraint( 0, 1, 0.1, PHASE_NOT_FIXED, instance );

        SmtLibWriter::addMaxConstraint( 1, { 2, 3, 4 }, PHASE_NOT_FIXED, 0, instance );

        PiecewiseLinearCaseSplit disjunct1;
        PiecewiseLinearCaseSplit disjunct2;
        PiecewiseLinearCaseSplit disjunct3;
        PiecewiseLinearCaseSplit disjunct4;
        PiecewiseLinearCaseSplit disjunct5;
        PiecewiseLinearCaseSplit disjunct6;

        Equation equation1 = Equation();
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -2, 1 );
        equation1._scalar = -4;

        disjunct1.addEquation( equation1 );
        disjunct1.storeBoundTightening( Tightening( 1, -2, Tightening::UB ) );

        disjunct2.storeBoundTightening( Tightening( 1, 2, Tightening::LB ) );
        disjunct2.storeBoundTightening( Tightening( 0, -1.5, Tightening::UB ) );

        SmtLibWriter::addDisjunctionConstraint( { disjunct1, disjunct2 }, instance );

        Equation equation2 = Equation();
        equation2.addAddend( 0, 0 );
        equation2.addAddend( -1, 1 );
        equation2._scalar = 1;

        Equation equation3 = Equation();
        equation3.addAddend( 0, 0 );
        equation3.addAddend( -1, 2 );
        equation3._scalar = 1;

        Equation equation4 = Equation();
        equation4.addAddend( 0, 0 );
        equation4.addAddend( -1, 3 );
        equation4._scalar = 1;

        disjunct3.addEquation( equation2 );
        disjunct5.addEquation( equation3 );
        disjunct6.addEquation( equation4 );

        SmtLibWriter::addDisjunctionConstraint( { disjunct3, disjunct5, disjunct6 }, instance );

        disjunct4.storeBoundTightening( Tightening( 1, 2, Tightening::LB ) );
        SmtLibWriter::addDisjunctionConstraint( { disjunct4 }, instance );
        SmtLibWriter::addDisjunctionConstraint( { disjunct3, disjunct4 }, instance );

        disjunct4.storeBoundTightening( Tightening( 1, 2, Tightening::LB ) );
        disjunct4.storeBoundTightening( Tightening( 0, -1.5, Tightening::UB ) );
        SmtLibWriter::addDisjunctionConstraint( { disjunct4 }, instance );

        disjunct3.addEquation( equation2 );
        disjunct3.addEquation( equation2 );
        SmtLibWriter::addDisjunctionConstraint( { disjunct3 }, instance );

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

        // Bounds
        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( <= x0 " ) + SmtLibWriter::signedValue( 1 ) + " ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( <= x1 " ) + SmtLibWriter::signedValue( 1 ) + " ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( >= x0 " ) + SmtLibWriter::signedValue( 1 ) + " ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( >= x1 " ) + SmtLibWriter::signedValue( -1 ) + " ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Tableau
        line = file->readLine( '\n' );
        expectedLine =
            String( "( assert ( = 0 ( + x0 ( * " ) + SmtLibWriter::signedValue( 2 ) + " x1 ) ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Relu
        line = file->readLine( '\n' );
        expectedLine = "( assert ( = x1 ( ite ( >= x0 0 ) x0 0 ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Sign
        line = file->readLine( '\n' );
        expectedLine = "( assert ( = x1 ( ite ( >= x0 0 ) 1 ( - 1 ) ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Absolute Value
        line = file->readLine( '\n' );
        expectedLine = "( assert ( = x1 ( ite ( >= x0 0 ) x0 ( - x0 ) ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // LeakyRelu
        line = file->readLine( '\n' );
        expectedLine = "( assert ( = x1 ( ite ( >= x0 0 ) x0 ( * 0.1 x0 ) ) ) )";
        TS_ASSERT_EQUALS( line, expectedLine );

        // Max
        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( => ( and ( >= x2 x3 ) ( >= x2 x4 ) ) ( = x1 x2 ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( => ( and ( >= x3 x2 ) ( >= x3 x4 ) ) ( = x1 x3 ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert ( => ( and ( >= x4 x2 ) ( >= x4 x3 ) ) ( = x1 x4 ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        // Disjunctions (several cases)
        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( or" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine =
            String( "( and ( = ( - 4 ) ( + x0 ( * ( - 2 ) x1 ) ) ) ( <= x1 ( - 2 ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( and ( >= x1 2 )( <= x0 ( - 1.5 ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( or" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( = 1 ( - x1 ) ) " );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( or" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( = 1 ( - x2 ) ) " );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( = 1 ( - x3 ) ) " );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( >= x1 2 )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( or" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( = 1 ( - x1 ) ) " );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( >= x1 2 )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( and ( >= x1 2 )( and ( >= x1 2 )( <= x0 ( - 1.5 ) ) ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( "( assert" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine =
            String( "( and ( = 1 ( - x1 ) ) ( and ( = 1 ( - x1 ) ) ( = 1 ( - x1 ) )  ) )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = String( " )" );
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( check-sat )";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file->readLine( '\n' );
        expectedLine = "( exit )";
        TS_ASSERT_EQUALS( line, expectedLine );
    }
};
