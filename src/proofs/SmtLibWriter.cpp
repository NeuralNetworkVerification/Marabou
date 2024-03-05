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

#include "SmtLibWriter.h"

const unsigned SmtLibWriter::SMTLIBWRITER_PRECISION =
    (unsigned)std::log10( 1 / GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );

void SmtLibWriter::addHeader( unsigned numberOfVariables, List<String> &instance )
{
    instance.append( "( set-logic QF_LRA )\n" );
    for ( unsigned i = 0; i < numberOfVariables; ++i )
        instance.append( "( declare-fun x" + std::to_string( i ) + " () Real )\n" );
}

void SmtLibWriter::addFooter( List<String> &instance )
{
    instance.append( "( check-sat )\n" );
    instance.append( "( exit )\n" );
}

void SmtLibWriter::addReLUConstraint( unsigned b,
                                      unsigned f,
                                      const PhaseStatus status,
                                      List<String> &instance )
{
    if ( status == PHASE_NOT_FIXED )
        instance.append( "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" +
                         std::to_string( b ) + " 0 ) x" + std::to_string( b ) + " 0 ) ) )\n" );
    else if ( status == RELU_PHASE_ACTIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) +
                         " ) )\n" );
    else if ( status == RELU_PHASE_INACTIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " 0 ) )\n" );
}

void SmtLibWriter::addSignConstraint( unsigned b,
                                      unsigned f,
                                      const PhaseStatus status,
                                      List<String> &instance )
{
    if ( status == PHASE_NOT_FIXED )
        instance.append( "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" +
                         std::to_string( b ) + " 0 ) 1 ( - 1 ) ) ) )\n" );
    else if ( status == SIGN_PHASE_POSITIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " 1 ) )\n" );
    else if ( status == SIGN_PHASE_NEGATIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " ( - 1 ) ) )\n" );
}

void SmtLibWriter::addAbsConstraint( unsigned b,
                                     unsigned f,
                                     const PhaseStatus status,
                                     List<String> &instance )
{
    if ( status == PHASE_NOT_FIXED )
        instance.append( "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" +
                         std::to_string( b ) + " 0 ) x" + std::to_string( b ) + " ( - x" +
                         std::to_string( b ) + " ) ) ) )\n" );
    else if ( status == ABS_PHASE_POSITIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) +
                         " ) )\n" );
    else if ( status == ABS_PHASE_NEGATIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " ( - x" + std::to_string( b ) +
                         " ) ) )\n" );
}

void SmtLibWriter::addMaxConstraint( unsigned f,
                                     const Set<unsigned> &elements,
                                     const PhaseStatus status,
                                     double maxVal,
                                     List<String> &instance )
{
    String assertRowLine;
    unsigned counter;
    unsigned size = elements.size();

    // f equals to some value (the value of maxVal)
    if ( status == MAX_PHASE_ELIMINATED )
        instance.append( String( "( assert ( = x" + std::to_string( f ) + " " ) +
                         signedValue( maxVal ) + " ) )\n" );

    // f equals to some element (maxVal is an index)
    else if ( status != PHASE_NOT_FIXED )
        instance.append( "( assert ( = x" + std::to_string( f ) + " x" +
                         std::to_string( (unsigned)maxVal ) + " ) )\n" );

    else
    {
        // For all elements (including eliminated), if an element is larger than all others, then f
        // = element
        for ( const auto &element : elements )
        {
            counter = 0;
            assertRowLine = "( assert ( =>";
            for ( auto const &otherElement : elements )
            {
                if ( otherElement == element )
                    continue;

                if ( counter < size - 2 )
                {
                    assertRowLine += " ( and";
                    ++counter;
                }

                assertRowLine += " ( >= x" + std::to_string( element ) + " x" +
                                 std::to_string( otherElement ) + " )";
            }

            for ( unsigned i = 0; i < size - 2; ++i )
                assertRowLine += String( " )" );

            assertRowLine +=
                " ( = x" + std::to_string( f ) + " x" + std::to_string( element ) + " )";

            instance.append( assertRowLine + " ) )\n" );
        }
    }
}

void SmtLibWriter::addDisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts,
                                             List<String> &instance )
{
    ASSERT( !disjuncts.empty() );

    unsigned size;
    instance.append( "( assert\n" );

    for ( const auto &disjunct : disjuncts )
    {
        if ( !( disjunct == disjuncts.back() ) )
            instance.append( "( or\n" );

        size = disjunct.getEquations().size() + disjunct.getBoundTightenings().size();
        ASSERT( size )
        // If disjucnt is a single equation or a single tightening, simply add them
        if ( size == 1 && disjunct.getEquations().size() == 1 )
            SmtLibWriter::addEquation( disjunct.getEquations().back(), instance );
        else if ( size == 1 && disjunct.getBoundTightenings().size() == 1 )
            SmtLibWriter::addTightening( disjunct.getBoundTightenings().back(), instance );
        else
        {
            // Otherwise, add the conjunction of the equations and\or tightenings
            unsigned counter = 0;

            for ( const auto &eq : disjunct.getEquations() )
            {
                if ( counter < size - 1 )
                    instance.append( "( and " );
                ++counter;
                SmtLibWriter::addEquation( eq, instance );
            }

            for ( const auto &bound : disjunct.getBoundTightenings() )
            {
                if ( counter < size - 1 )
                    instance.append( "( and " );
                ++counter;

                SmtLibWriter::addTightening( bound, instance );
            }
        }

        for ( unsigned i = 0; i < size - 1; ++i )
            instance.append( " )" );
        instance.append( "\n" );
    }

    size = disjuncts.size();
    for ( unsigned i = 0; i < size; ++i )
        instance.append( String( " )" ) );

    instance.append( "\n" );
}

void SmtLibWriter::addLeakyReLUConstraint( unsigned b,
                                           unsigned f,
                                           double slope,
                                           const PhaseStatus status,
                                           List<String> &instance )
{
    if ( status == PHASE_NOT_FIXED )
        instance.append( String( "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" +
                                 std::to_string( b ) + " 0 ) x" + std::to_string( b ) + " ( * " ) +
                         signedValue( slope ) + " x" + std::to_string( b ) + " ) ) ) )\n" );
    else if ( status == RELU_PHASE_ACTIVE )
        instance.append( "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) +
                         " ) )\n" );
    else if ( status == RELU_PHASE_INACTIVE )
        instance.append(
            String( "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) ) +
            signedValue( -slope ) + ") )\n" );
}

void SmtLibWriter::addTableauRow( const SparseUnsortedList &row, List<String> &instance )
{
    unsigned size = row.getSize();

    // Avoid adding a redundant last element
    auto it = --row.end();
    if ( std::isnan( it->_value ) || FloatUtils::isZero( it->_value ) )
        --size;

    if ( !size )
        return;

    unsigned counter = 0;
    String assertRowLine = "( assert ( = 0";
    auto entry = row.begin();

    for ( ; entry != row.end(); ++entry )
    {
        if ( FloatUtils::isZero( entry->_value ) )
            continue;

        if ( counter != size - 1 )
            assertRowLine += String( " ( + " );
        else
            assertRowLine += String( " " );

        // Coefficients +-1 can be dropped
        if ( entry->_value == 1 )
            assertRowLine += String( "x" ) + std::to_string( entry->_index );
        else if ( entry->_value == -1 )
            assertRowLine += String( "( - x" ) + std::to_string( entry->_index ) + " )";
        else
            assertRowLine += String( "( * " ) + signedValue( entry->_value ) + " x" +
                             std::to_string( entry->_index ) + " )";

        ++counter;
    }

    for ( unsigned i = 0; i < counter + 1; ++i )
        assertRowLine += String( " )" );

    instance.append( assertRowLine + "\n" );
}

void SmtLibWriter::addGroundUpperBounds( Vector<double> &bounds, List<String> &instance )
{
    unsigned n = bounds.size();
    for ( unsigned i = 0; i < n; ++i )
        instance.append( String( "( assert ( <= x" + std::to_string( i ) ) + String( " " ) +
                         signedValue( bounds[i] ) + " ) )\n" );
}

void SmtLibWriter::addGroundLowerBounds( Vector<double> &bounds, List<String> &instance )
{
    unsigned n = bounds.size();
    for ( unsigned i = 0; i < n; ++i )
        instance.append( String( "( assert ( >= x" + std::to_string( i ) ) + String( " " ) +
                         signedValue( bounds[i] ) + " ) )\n" );
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
    std::stringstream s;
    s << std::fixed << std::setprecision( SMTLIBWRITER_PRECISION ) << abs( val );
    return val >= 0 ? String( s.str() ).trimZerosFromRight()
                    : String( "( - " + s.str() ).trimZerosFromRight() + " )";
}

void SmtLibWriter::addEquation( const Equation &eq, List<String> &instance )
{
    unsigned size = eq._addends.size();

    if ( !size )
        return;

    unsigned counter = 0;

    String assertRowLine = "";

    if ( eq._type == Equation::EQ )
        assertRowLine += "( = ";
    else if ( eq._type == Equation::LE )
        // Scalar should be >= than sum of addends
        assertRowLine += "( >= ";
    else
        // Scalar should be <= than sum of addends
        assertRowLine += "( <= ";

    assertRowLine += signedValue( eq._scalar );

    for ( const auto &addend : eq._addends )
    {
        if ( FloatUtils::isZero( addend._coefficient ) )
            continue;

        if ( !( addend == eq._addends.back() ) )
            assertRowLine += String( " ( + " );
        else
            assertRowLine += String( " " );


        // Coefficients +-1 can be dropped
        if ( addend._coefficient == 1 )
            assertRowLine += String( "x" ) + std::to_string( addend._variable );
        else if ( addend._coefficient == -1 )
            assertRowLine += String( "( - x" ) + std::to_string( addend._variable ) + " )";
        else
            assertRowLine += String( "( * " ) + signedValue( addend._coefficient ) + " x" +
                             std::to_string( addend._variable ) + " )";

        ++counter;
    }

    for ( unsigned i = 0; i < counter; ++i )
        assertRowLine += String( " )" );

    instance.append( assertRowLine + " " );
}

void SmtLibWriter::addTightening( Tightening bound, List<String> &instance )
{
    if ( bound._type == Tightening::LB )
        instance.append( String( "( >= x" ) + std::to_string( bound._variable ) + " " +
                         signedValue( bound._value ) + " )" );
    else
        instance.append( String( "( <= x" + std::to_string( bound._variable ) ) + String( " " ) +
                         signedValue( bound._value ) + " )" );
}