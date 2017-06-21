/*********************                                                        */
/*! \file FloatUtils.h
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
#include "GlobalConfiguration.h"

const double GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS = 0.0000000001;

void GlobalConfiguration::print()
{
    printf( "****************************\n" );
    printf( "*** Global Configuraiton ***\n" );
    printf( "****************************\n" );
    printf( "  DEFAULT_EPSILON_FOR_COMPARISONS: %lf\n", DEFAULT_EPSILON_FOR_COMPARISONS );
    printf( "****************************\n" );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
