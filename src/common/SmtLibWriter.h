/*********************                                                        */
/*! \file SmtLibWriter.h
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

#ifndef MARABOU_SMTLIBWRITER_H
#define MARABOU_SMTLIBWRITER_H
#include "List.h"
#include "MString.h"
#include "SparseUnsortedList.h"
#include "vector"
#include <fstream>


/*
 * A class responsible for writing instances of LP+PLC into SMTLIB format
 */
class SmtLibWriter
{
public:

	/*
 	* Adds a SMTLIB header to the SMTLIB instance with n variables
 	*/
	static void addHeader( unsigned n, List<String> &instance )
	{
		instance.append( "( set-logic QF_LRA )\n" );
		for ( unsigned i = 0; i < n; ++i )
			instance.append( "( declare-fun x" + std::to_string( i ) + " () Real )\n" );
	}

	/*
 	* Adds a SMTLIB footer to the SMTLIB instance
 	*/
	static void addFooter( List<String> &instance )
	{
		instance.append(  "( check-sat )\n" );
		instance.append(  "( exit )\n" );
	}

	/*
	 * Adds a line representing ReLU constraint, in SMTLIB format, to the SMTLIB instance
	 */
	static void addReLUConstraint( unsigned b, unsigned f, PhaseStatus status, List<String> &instance )
	{
		if ( status == PHASE_NOT_FIXED )
			instance.append(  "( assert ( = x" + std::to_string( f ) + " ( ite ( >= x" + std::to_string( b ) + " 0) x" + std::to_string( b )+ " 0 ) ) )\n" );
		else if ( status == RELU_PHASE_ACTIVE )
			instance.append(  "( assert ( = x" + std::to_string( f ) + " x" + std::to_string( b ) + " ) )\n" );
		else if ( status == RELU_PHASE_INACTIVE )
			instance.append(  "( assert ( = x" + std::to_string( f ) + " 0 ) )\n" );
	}

	/*
 	* Adds a line representing a Tableau Row, in SMTLIB format, to the SMTLIB instance
 	*/
	static void addTableauRow( const SparseUnsortedList &row, List<String> &instance )
	{
		unsigned size = row.getSize(), counter = 0;
		String assertRowLine = "( assert ( = 0";
		for ( auto &entry : row )
		{
			if ( counter != size - 1 )
				assertRowLine += " ( + ( * " + signedValue( entry._value ) + " x" + std::to_string( entry._index ) + " )";
			else
				assertRowLine += " ( * " + signedValue( entry._value ) + " x" + std::to_string( entry._index ) + " )";
			++counter;
		}

		assertRowLine += std::string( counter + 1, ')' );
		instance.append( assertRowLine + "\n" );
	}

	/*
 	* Adds lines representing a the ground upper bounds, in SMTLIB format, to the SMTLIB instance
 	*/
	static void addGroundUpperBounds( const std::vector<double> &bounds,List<String> &instance )
	{
		unsigned n = bounds.size();
		for ( unsigned i = 0; i < n; ++i)
			instance.append( " ( assert ( <= x" + std::to_string( i ) + " " + signedValue( bounds[i] ) + " ) )\n" );
	}

	/*
 	* Adds lines representing a the ground lower bounds, in SMTLIB format, to the SMTLIB instance
 	*/
	static void addGroundLowerBounds( const std::vector<double> &bounds, List<String> &instance )
	{
		unsigned n = bounds.size();
		for ( unsigned i = 0; i < n; ++i)
			instance.append( " ( assert ( >= x" + std::to_string( i ) + " " + signedValue( bounds[i] ) + " ) )\n" );
	}

	/*
	 * Write the instances to files
	 */
	static void writeInstanceToFile( const std::string &directory, unsigned delegationNumber , const List<String>& instance )
	{
		std::ofstream file ( directory + "delegated" + std::to_string( delegationNumber ) + ".smtlib");
		for ( const String &s : instance )
			file << s;
		file.close();
	}

	static std::string signedValue( double val )
	{
		return val > 0 ? std::to_string( val ) : "( - " + std::to_string( abs( val ) ) + " )";
	}
};

#endif //MARABOU_SMTLIBWRITER_H