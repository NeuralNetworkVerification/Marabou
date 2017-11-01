/*********************                                                        */
/*! \file main_regress.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include <cstdio>

#include "RegressUtils.h"
#include "ReluplexError.h"

#include "acas_1_1_no_constraints.h"
#include "acas_1_1_fixed_input.h"
#include "acas_2_2_fixed_input.h"
#include "lp_feasible_1.h"
#include "lp_feasible_2.h"
#include "lp_infeasible_1.h"
#include "max_feasible_1.h"
#include "max_infeasible_1.h"
#include "relu_feasible_1.h"

void lps()
{
    printTitle( "Pure LP / Sat" );
    Lp_Feasible_1 lpf1;
    lpf1.run();

    Lp_Feasible_2 lpf2;
    lpf2.run();

    // Lp_Feasible_3 lpf3;
    // lpf3.run();

    printTitle( "Pure LP / Unsat" );
    Lp_Infeasible_1 lpi1;
    lpi1.run();
}

void relus()
{
    printTitle( "ReLUs / Sat" );
    Relu_Feasible_1 rf1;
    rf1.run();

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

int main()
{
    try
	{
		lps();

    	relus();

        max();

        printf( "\n\n" );
	}
	catch( const ReluplexError &e )
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
