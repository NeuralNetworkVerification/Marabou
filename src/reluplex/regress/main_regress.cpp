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

#include "lp_feasible_1.h"
#include "lp_feasible_2.h"
// #include "lp_feasible_3.h"

#include "lp_infeasible_1.h"

#include "relu_feasible_1.h"

#include "max_feasible_1.h"
#include "max_infeasible_1.h"

#include "ReluplexError.h"

void lps()
{
    // Sat
    Lp_Feasible_1 lpf1;
    lpf1.run();

    Lp_Feasible_2 lpf2;
    lpf2.run();
    
    // Lp_Feasible_3 lpf3;
    // lpf3.run();

    // Unsat
    Lp_Infeasible_1 lpi1;
    lpi1.run();
}

void relus()
{
    Relu_Feasible_1 rf1;
    rf1.run();
}

void max()
{
	max_feasible_1 mf1;
	mf1.run();

	max_infeasible_1 mf2;
	mf2.run();
}

int main()
{
    try
	{
		lps();

    	relus();

        // Temporarily disabling
		// max();
	}
	catch( const ReluplexError &e )
	{
		printf("%u", e.getCode() );
	}
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
