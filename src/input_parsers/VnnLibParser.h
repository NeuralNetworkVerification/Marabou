/*********************                                                        */
/*! \file VnnLibParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A parser for properties encoded in VNN-LIB format.

**/

#ifndef _VnnLibParser_h_
#define _VnnLibParser_h_

#include "IQuery.h"
#include "MString.h"
#include "Map.h"
#include "Vector.h"

class VnnLibParser
{
public:
    void parse( const String &vnnlibFilePath, IQuery &inputQuery );

private:
    class Term
    {
    public:
        enum TermType {
            CONST,
            VARIABLE,
            ADD,
            SUB,
            MUL
        };

        TermType _type;
        String _value;
        Vector<Term> _args;
    };

    Map<String, unsigned int> _varMap;

    void parseVnnlib( const String &vnnlibContent, IQuery &inputQuery );

    int parseScript( const Vector<String> &tokens, IQuery &inputQuery );

    int parseCommand( int index, const Vector<String> &tokens, IQuery &inputQuery );

    int parseDeclareConst( int index, const Vector<String> &tokens, IQuery &inputQuery );

    int parseAssert( int index, const Vector<String> &tokens, IQuery &inputQuery );

    int parseCondition( int index, const Vector<String> &tokens, List<Equation> &equations );

    int parseTerm( int index, const Vector<String> &tokens, Term &term );

    int parseComplexTerm( int index, const Vector<String> &tokens, VnnLibParser::Term &term );

    double
    processAddConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs = false );

    double
    processSubConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs = false );

    double
    processMulConstraint( const VnnLibParser::Term &term, Equation &equation, bool isRhs = false );

    Equation processLeConstraint( const VnnLibParser::Term &arg1, const VnnLibParser::Term &arg2 );
};

#endif //_VnnLibParser_h_
