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
#include "OnnxParser.h"
#include "Engine.h"
#include "InputQuery.h"

class Marabou
{
public:
    Marabou( InputQuery& inputQuery );
    ~Marabou();

    /*
      Entry point of this class
    */
    void run();

private:
    InputQuery _inputQuery;

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
      Export assignment as per Options
     */
    void exportAssignment() const;

    /*
      Import assignment for debugging as per Options
     */
    void importDebuggingSolution();

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
