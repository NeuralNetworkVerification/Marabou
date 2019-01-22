/*********************                                                        */
/*! \file BerkeleyNeuralNetwork.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __BerkeleyNeuralNetwork_h__
#define __BerkeleyNeuralNetwork_h__

#include "File.h"
#include "MString.h"
#include "Set.h"
#include "Vector.h"
#include "Map.h"

#include <cassert>
#include <iomanip>
#include <sstream>

class BerkeleyNeuralNetwork
{
public:
    class Equation
    {
    public:
        struct RhsPair
        {
            unsigned _var;
            double _coefficient;
        };

        unsigned _lhs;
        List<RhsPair> _rhs;
        double _constant;
        unsigned _index;

        Equation();
        void dump();
    };

    /*
      Open the input file for reading.
    */
    BerkeleyNeuralNetwork( const String &path );

    /*
      Parse the input file.
    */
    void parseFile();

    unsigned getNumVariables() const;
    unsigned getNumEquations() const;
    unsigned getNumInputVariables() const;
    unsigned getNumOutputVariables() const;
    unsigned getNumReluNodes() const;
    Set<unsigned> getInputVariables() const;
    Set<unsigned> getOutputVariables() const;
    Map<unsigned, unsigned> getFToB() const;
    List<Equation> getEquations() const;

private:
    File _file;

    /*
      Maps from b to f variables, and vice-versa.
    */
    Map <unsigned, unsigned> _bToF;
    Map <unsigned, unsigned> _fToB;

    /*
      All equations extracted from the file.
    */
    List<Equation> _equations;

    /*
      The collections of all variables encountered in the file.
    */
    Set<unsigned> _allVars;
    Set<unsigned> _allLhsVars;
    Set<unsigned> _inputVars;
    unsigned _maxVar;
    Set<unsigned> _allRhsVars;
    Set<unsigned> _outputVars;

    /*
      Process a single line of the input file, either a ReLU or
      an equation.
    */
    void processLine( const String &line );
    void processReluLine( const String &line );
    void processEquationLine( const String &line );

    /*
      Extract a variable index from a string.
    */
    unsigned varStringToUnsigned( String varString );

    /*
      Store the newly-encountered variable in the recrods.
    */
    void processVar( unsigned var );
};

#endif // __BerkeleyNeuralNetwork_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
