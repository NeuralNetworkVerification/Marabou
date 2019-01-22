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

#include "Map.h"
#include "Equation.h"
#include "Set.h"

class InputQuery;
class String;

class MpsParser
{
public:
    enum RowType {
        E = 0, // equality
        L, // less than or equal
        G, // greater than or equal
        N, // no restriction
    };

    MpsParser( const char *filename );
    void generateQuery( InputQuery &inputQuery );

    unsigned getNumVars() const;
    unsigned getNumVarsInclAux() const;
    unsigned getNumEqns() const;

    // printables
    String getEqnName( unsigned index ) const;
    String getVarName( unsigned index ) const;
    String getUpperBound( unsigned index ) const;
    String getLowerBound( unsigned index ) const;

    String getEqnLB( unsigned index ) const;
    String getEqnUB( unsigned index ) const;
    String getEqnValue( unsigned index, const InputQuery &inputQuery ) const;
    //    Set<String> getVarNames() const;
    //    unsigned getVarIndex( const String &name ) const;

//    unsigned getInputVariable( unsigned index ) const;
//    unsigned getOutputVariable( unsigned index ) const;
//    unsigned getBVariable( unsigned layer, unsigned index ) const;
//    unsigned getFVariable( unsigned layer, unsigned index ) const;
//    unsigned getAuxVariable( unsigned layer, unsigned index ) const;

private:
    void parse( const char *filename );
    void parseRow( const char *token );
    void parseColumn( const char *token );
    void parseRhs( const char *token );
    void parseRanges( const char *token );
    void parseBounds( const char *token );
    void parseRemainingBounds();

    void setBounds( InputQuery &inputQuery );
    void setEqns( InputQuery &inputQuery );
    void setEqn( Equation &eqn, unsigned eqnIndex, unsigned auxVarIndex );

    const char *DELIM = "\n\t ";
    unsigned _numVars = 0;
    unsigned _numEqns = 0;
    Map<String, unsigned> _eqnToIndex;
    Map<unsigned, String> _indexToEqn;
    Map<unsigned, unsigned> _eqnToRowType;
    Map<unsigned, Map<unsigned, double>> _eqnToCoeffs; // Eqn: {Var: Coeff, ...}
    Map<unsigned, double> _eqnToRhs;

    Map<String, unsigned> _varToIndex;
    Map<unsigned, String> _indexToVar;
    Map<unsigned, double> _varToUpperBounds;
    Map<unsigned, double> _varToLowerBounds;

    String ftos( double val ) const;
};

#endif // __MpsParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
