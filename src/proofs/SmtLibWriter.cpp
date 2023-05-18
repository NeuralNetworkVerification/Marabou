/*********************                                                        */
/*! \file SmtLibWriter.cpp
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

#include <iomanip>
#include "SmtLibWriter.h"

void SmtLibWriter::addHeader( unsigned numberOfVariables, List<String> &instance )
{
    instance.append( "( set-logic QF_LRA )\n" );
    for ( unsigned i = 0; i < numberOfVariables; ++i )
        instance.append( "( declare-fun x" + std::to_string( i ) + " () Real )\n" );
}

void SmtLibWriter::addFooter( List<String> &instance )
{
    instance.append(  "( check-sat )\n" );
    instance.append(  "( exit )\n" );
}

void SmtLibWriter::addReLUConstraint( unsigned b, unsigned f, const PhaseStatus status, List<String> &instance )
{
    if ( status == PHASE_NOT_FIXED )
        instance.append(  "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" + std::to_string( b ) + " 0 ) x" + std::to_string( b )+ " 0 ) ) )\n" );
    else if ( status == RELU_PHASE_ACTIVE )
        instance.append(  "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) + " ) )\n" );
    else if ( status == RELU_PHASE_INACTIVE )
        instance.append(  "( assert ( = x" + std::to_string( f ) + " 0 ) )\n" );
}

void SmtLibWriter::addTableauRow( const SparseUnsortedList &row, List<String> &instance )
{
    unsigned size = row.getSize();
    unsigned counter = 0;
    String assertRowLine = "( assert ( = 0";
    auto entry = row.begin();

    for ( ; entry != row.end(); ++entry )
    {
        if ( FloatUtils::isZero( entry->_value ) )
            continue;

        if ( counter == size - 1 )
            break;

        assertRowLine += String( " ( + ( * " ) + signedValue( entry->_value ) + " x" + std::to_string( entry->_index ) + " )";
        ++counter;
    }

    // Add last element
    assertRowLine += String( " ( * " ) + signedValue( entry->_value ) + " x" + std::to_string( entry->_index ) + " )";

    for ( unsigned i = 0; i < counter + 2 ; ++i )
        assertRowLine += String( " )" );

    instance.append( assertRowLine + "\n" );
}

void SmtLibWriter::addGroundUpperBounds( Vector<double> &bounds, List<String> &instance )
{
    unsigned n = bounds.size();
    for ( unsigned i = 0; i < n; ++i )
        instance.append( String( "( assert ( <= x" + std::to_string( i ) ) + String( " " ) + signedValue( bounds[i] ) + " ) )\n" );
}

void SmtLibWriter::addGroundLowerBounds( Vector<double> &bounds, List<String> &instance )
{
    unsigned n = bounds.size();
    for ( unsigned i = 0; i < n; ++i )
        instance.append( String( "( assert ( >= x" + std::to_string( i ) ) + String( " " ) + signedValue( bounds[i] ) + " ) )\n" );
}

void SmtLibWriter::writeInstanceToFile( IFile &file, const List<String> &instance )
{
    file.open( File::MODE_WRITE_TRUNCATE );

    for ( const String &s : instance )
        file.write( s );

    file.close();
}

String SmtLibWriter::signedValue( double val )
{
    unsigned precision = ( unsigned ) std::log10( 1 / GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    std::stringstream s;
    s << std::fixed << std::setprecision( precision ) << abs( val );
    return val >= 0 ? String( s.str() ).trimZerosFromRight() : String( "( - " + s.str() ).trimZerosFromRight() + " )";
}