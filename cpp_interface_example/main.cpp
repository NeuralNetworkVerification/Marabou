/*********************                                                        */
/*! \file relu_feasible_2.h
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

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "ReluConstraint.h"

int main()
{
    /*
      An example on a small network with ReLU activations

                ReLU
            x1 ------> x2
         1 /             \ 1
          /               \
     --> x0                x5 -->
          \               /
        -1 \    ReLU     / 1
            x3 ------> x4


      Where the input and output constraints are:

        0   <= x0 <= 1
        1/2 <= x1 <= 1
    */

    InputQuery inputQuery;
    inputQuery.setNumberOfVariables( 6 );

    inputQuery.setLowerBound( 0, 0 );
    inputQuery.setUpperBound( 0, 1 );

    inputQuery.setLowerBound( 5, 0.5 );
    inputQuery.setUpperBound( 5, 1 );

    Equation equation1;  // x0 - x1 = 0
    equation1.addAddend( 1, 0 );
    equation1.addAddend( -1, 1 );
    equation1.setScalar( 0 );
    inputQuery.addEquation( equation1 );

    Equation equation2;  // x0 + x3 = 0
    equation2.addAddend( 1, 0 );
    equation2.addAddend( 1, 3 );
    equation2.setScalar( 0 );
    inputQuery.addEquation( equation2 );

    Equation equation3;  // x2 + x4 - x5 = 0
    equation3.addAddend( 1, 2 );
    equation3.addAddend( 1, 4 );
    equation3.addAddend( -1, 5 );
    equation3.setScalar( 0 );
    inputQuery.addEquation( equation3 );

    ReluConstraint *relu1 = new ReluConstraint( 1, 2 );
    ReluConstraint *relu2 = new ReluConstraint( 3, 4 );

    inputQuery.addPiecewiseLinearConstraint( relu1 );
    inputQuery.addPiecewiseLinearConstraint( relu2 );

    Engine engine;
    if ( engine.processInputQuery( inputQuery ) && engine.solve() )
    {
        printf( "\nQuery is SAT!\n\n" );

        engine.extractSolution( inputQuery );
        printf( "Printing out assignment...\n" );
        for ( unsigned i = 0; i < 6; ++i )
            printf( "\tx%u = %.2lf\n", i, inputQuery.getSolutionValue( i ) );
        printf( "\n" );
    }
    else
        printf( "\nQuery is UNSAT!\n\n" );

    return 0;
};

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
