/*********************                                                        */
/*! \file Marabou.h
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

#ifndef __Marabou_h__
#define __Marabou_h__

#include "AcasParser.h"
#include "Engine.h"
#include "InputQuery.h"

class Marabou
{
public:
    Marabou( unsigned verbosity = 2 );
    ~Marabou();

    /*
      Entry point of this class
    */
    void run();

private:
    InputQuery _inputQuery;

    /*
      Extract the options and input files (network and property), and
      use them to generate the input query
    */
    void prepareInputQuery();
    void extractSplittingThreshold();

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
