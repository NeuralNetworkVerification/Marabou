/*********************                                                        */
/*! \file Lp_Feasible_1.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Lp_Feasible_1_h__
#define __Lp_Feasible_1_h__

#include "Engine.h"
#include "InputQuery.h"

class Lp_Feasible_1
{
public:
    void run()
    {
        // Simple satisfiable query:
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0 + 2x1 -x2 <= 11 --> x0 + 2x1 - x2 + x3 = 11, x3 >= 0

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 4 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );

        InputQuery::Equation equation;
        equation.addAddend( 1, 0 );
        equation.addAddend( 2, 1 );
        equation.addAddend( -1, 2 );
        equation.addAddend( 1, 3 );
        equation.setScalar( 11 );
        equation.markAuxiliaryVariable( 3 );
        inputQuery.addEquation( equation );

        Engine engine;
        engine.processInputQuery( inputQuery );
        engine.solve();
    }
};

#endif // __Lp_Feasible_1_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
