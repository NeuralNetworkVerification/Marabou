/*********************                                                        */
/*! \file MpsParser.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Rachel Lim, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __MpsParser_h__
#define __MpsParser_h__

#include "Equation.h"
#include "Map.h"
#include "Set.h"

#define MPS_LOG(x, ...) LOG(GlobalConfiguration::MPS_PARSER_LOGGING, "MpsParser: %s\n", x)

class InputQuery;
class String;

class MpsParser
{
public:
    enum RowType {
        EQ = 0,
        LE,
        GE,
    };

    MpsParser( const String &path );

    // Extract an input query from the input file
    void generateQuery( InputQuery &inputQuery ) const;

    // Getters
    unsigned getNumVars() const;
    unsigned getNumEquations() const;
    String getEquationName( unsigned index ) const;
    String getVarName( unsigned index ) const;
    double getUpperBound( unsigned index ) const;
    double getLowerBound( unsigned index ) const;

private:
    // Helpers for parsing the various section of the file
    void parse( const String &path );
    void parseRow( const String &line );
    void parseColumn( const String &line );
    void parseRhs( const String &line );
    void parseBounds( const String &line );
    void setRemainingBounds();

    // Helpers for preparing the input query
    void populateBounds( InputQuery &inputQuery ) const;
    void populateEquations( InputQuery &inputQuery ) const;
    void populateEquation( Equation &equation, unsigned index ) const;

    // Number of equations and variables
    unsigned _numRows;
    unsigned _numVars;

    // Various mappings
    Map<String, unsigned> _equationNameToIndex;
    Map<unsigned, String> _equationIndexToName;
    Map<unsigned, unsigned> _equationIndexToRowType;
    Map<unsigned, Map<unsigned, double>> _equationIndexToCoefficients;
    Map<unsigned, double> _equationIndexToRhs;
    Map<String, unsigned> _variableNameToIndex;
    Map<unsigned, String> _variableIndexToName;
    Map<unsigned, double> _varToUpperBounds;
    Map<unsigned, double> _varToLowerBounds;
};

#endif // __MpsParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
