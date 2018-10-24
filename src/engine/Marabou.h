/*********************                                                        */
/*! \file Marabou.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Marabou_h__
#define __Marabou_h__

#include "AcasParser.h"
#include "Engine.h"
#include "InputQuery.h"

class Marabou
{
public:
    Marabou();
    ~Marabou();

    /*
      Entry point of this class
    */
    void run( int argc, char **argv );

private:
    InputQuery _inputQuery;

    /*
      Extract the input files: network and property, and use them
      to generate the input query
    */
    void prepareInputQuery();

    /*
      Invoke the engine to solve the input query
    */
    void solveQuery();

    /*
      Display the results
    */
    void displayResults( unsigned long long microSecondsElapsed ) const;

    /*
      ACAS network parser
    */
    AcasParser *_acasParser;

    /*
      The solver
    */
    Engine _engine;
};

#endif // __Marabou_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
