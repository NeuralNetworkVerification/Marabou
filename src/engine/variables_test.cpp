/*********************                                                        */
/*! \file main.cpp
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

#include "Marabou.h"


void test()
{
    InputQuery inputQuery;
    inputQuery.setNumberOfVariables( 20 );

    /* INPUT */
    inputQuery.setLowerBound(0, 0.1 );
    inputQuery.setUpperBound(0, 0.1 );
    inputQuery.setLowerBound(1, 0.2 );
    inputQuery.setUpperBound(1, 0.2 );
    inputQuery.setLowerBound(2, 0.3 );
    inputQuery.setUpperBound(2, 0.3 );
    inputQuery.setLowerBound(3, 0.4 );
    inputQuery.setUpperBound(3, 0.4 );

    /* // RNN CELL 1 */
    /* inputQuery.setLowerBound(4, 0 ); */
    /* inputQuery.setUpperBound(4, 10 ); */
    /* inputQuery.setLowerBound(5, 0 ); */
    /* inputQuery.setUpperBound(5, 500000); */
    /* inputQuery.setLowerBound(6, -500000 ); */
    /* inputQuery.setUpperBound(6, 500000 ); */
    /* inputQuery.setLowerBound(7, 0 ); */
    /* inputQuery.setUpperBound(7, 500000 ); */

    /* // RNN CELL 2 */
    /* inputQuery.setLowerBound(8, 0 ); */
    /* inputQuery.setUpperBound(8, 10 ); */
    /* inputQuery.setLowerBound(9, 0 ); */
    /* inputQuery.setUpperBound(9, 500000); */
    /* inputQuery.setLowerBound(10, -500000 ); */
    /* inputQuery.setUpperBound(10, 500000 ); */
    /* inputQuery.setLowerBound(11, 0 ); */
    /* inputQuery.setUpperBound(11, 500000 ); */

    /* // RNN CELL 3 */
    /* inputQuery.setLowerBound(12, 0 ); */
    /* inputQuery.setUpperBound(12, 10 ); */
    /* inputQuery.setLowerBound(13, 0 ); */
    /* inputQuery.setUpperBound(13, 500000); */
    /* inputQuery.setLowerBound(14, -500000 ); */
    /* inputQuery.setUpperBound(14, 500000 ); */
    /* inputQuery.setLowerBound(15, 0 ); */
    /* inputQuery.setUpperBound(15, 500000 ); */

    /* // RNN CELL 4 */
    /* inputQuery.setLowerBound(16, 0 ); */
    /* inputQuery.setUpperBound(16, 10 ); */
    /* inputQuery.setLowerBound(17, 0 ); */
    /* inputQuery.setUpperBound(17, 500000); */
    /* inputQuery.setLowerBound(18, -500000 ); */
    /* inputQuery.setUpperBound(18, 500000 ); */
    /* inputQuery.setLowerBound(19, 0 ); */
    /* inputQuery.setUpperBound(19, 500000 ); */

    ReluConstraint *relu1 = new ReluConstraint( 6, 7 );
    ReluConstraint *relu2 = new ReluConstraint( 10, 11 );
    ReluConstraint *relu3 = new ReluConstraint( 14, 15 );
    ReluConstraint *relu4 = new ReluConstraint( 18, 19 );
    inputQuery.addPiecewiseLinearConstraint( relu1 );
    inputQuery.addPiecewiseLinearConstraint( relu2 );
    inputQuery.addPiecewiseLinearConstraint( relu3 );
    inputQuery.addPiecewiseLinearConstraint( relu4 );

    Equation equation1;
    equation1.addAddend( -0.03, 0 );
    equation1.addAddend( -1, 6 );
    inputQuery.addEquation( equation1 );

    Equation equation2;
    equation2.addAddend( -0.06, 0 );
    equation2.addAddend( -0.05, 1 );
    equation2.addAddend( -1, 10 );
    inputQuery.addEquation( equation2 );

    Equation equation3;
    equation3.addAddend( -0.01, 0 );
    equation3.addAddend( -1, 14 );
    inputQuery.addEquation( equation3 );

    Equation equation4;
    equation4.addAddend( -0.0006, 1 );
    equation4.addAddend( -1, 18 );
    inputQuery.addEquation( equation4 );

    Equation *eq_eq = new Equation();
    eq_eq->addAddend( 1, 4 );
    eq_eq->addAddend( -1, 8 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 4 );
    eq_eq->addAddend( -1, 12 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;
    
    eq_eq = new Equation();
    eq_eq->addAddend( 1, 8 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 16 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 5 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 9 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 13 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    eq_eq = new Equation();
    eq_eq->addAddend( 1, 17 );
    eq_eq->setScalar( 0 );
    inputQuery.addEquation( *eq_eq);
    delete eq_eq;

    inputQuery.dump();
    Statistics retStats;
    Engine engine;

    printf("processInputQuery\n");
    if (!engine.processInputQuery( inputQuery )) {
        printf("processInputQuery Fail\n");
        return;
    }
}

int main( )
{
    printf("starting test\n");
    test();
    printf("after test\n");
    exit(0);
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
