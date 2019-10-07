/*********************                                                        */
/*! \file main_regress.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include <cstdio>

#include "RegressUtils.h"
#include "MarabouError.h"

#include "acas_1_1_fixed_input.h"
#include "acas_1_1_no_constraints.h"
#include "acas_2_2_fixed_input.h"
#include "lp_feasible_1.h"
#include "lp_feasible_2.h"
#include "lp_infeasible_1.h"
#include "max_feasible_1.h"
#include "max_infeasible_1.h"
#include "max_relu_feasible_1.h"
#include "mps_lp_feasible_1.h"
#include "mps_lp_infeasible_1.h"
#include "relu_feasible_1.h"
#include "relu_feasible_2.h"

void lps()
{
    printTitle( "Pure LP / Sat" );
    Lp_Feasible_1 lpf1;
    lpf1.run();

    Lp_Feasible_2 lpf2;
    lpf2.run();

    Mps_Lp_Feasible_1 mps_lpf1;
    mps_lpf1.run();

    printTitle( "Pure LP / Unsat" );
    Lp_Infeasible_1 lpi1;
    lpi1.run();

    Mps_Lp_Infeasible_1 mps_lpi1;
    mps_lpi1.run();
}

void relus()
{
    printTitle( "ReLUs / Sat" );
    Relu_Feasible_1 rf1;
    rf1.run();

    Relu_Feasible_2 rf2;
    rf2.run();

    Acas_1_1_Fixed_Input acas_1_1_fixed_input;
    acas_1_1_fixed_input.run();

    Acas_2_2_Fixed_Input acas_2_2_fixed_input;
    acas_2_2_fixed_input.run();

    Acas_1_1_No_Constraints acas_1_1_no_constraints;
    acas_1_1_no_constraints.run();
}

void max()
{
    printTitle( "Maxes / Sat" );
	Max_Feasible_1 mf1;
	mf1.run();

    printTitle( "Maxes / Unsat" );

	Max_Infeasible_1 mfi1;
	mfi1.run();
}

void max_relu()
{
    printTitle( "Max_Relu / Sat" );
    Max_Relu_Feasible_1 mrf1;
    mrf1.run();
}

int main()
{
    try
    {
        lps();

    	relus();

        max();

        max_relu();

        printf( "\n\n" );
	}
	catch( const MarabouError &e )
	{
		printf( "%u", e.getCode() );
	}
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
