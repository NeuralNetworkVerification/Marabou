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
const unsigned GlobalConfiguration::DEFAULT_DOUBLE_TO_STRING_PRECISION = 10;
const unsigned GlobalConfiguration::REFACTORIZATION_THRESHOLD = 100;
const unsigned GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY = 100;
const double GlobalConfiguration::NUMERICAL_STABILITY_CONSTANT = 0.0001;

// Logging
const bool GlobalConfiguration::ENGINE_LOGGING = false;
const bool GlobalConfiguration::TABLEAU_LOGGING = false;
const bool GlobalConfiguration::SMT_CORE_LOGGING = false;

void GlobalConfiguration::print()
{
    printf( "****************************\n" );
    printf( "*** Global Configuraiton ***\n" );
    printf( "****************************\n" );
    printf( "  DEFAULT_EPSILON_FOR_COMPARISONS: %lf\n", DEFAULT_EPSILON_FOR_COMPARISONS );
    printf( "  DEFAULT_DOUBLE_TO_STRING_PRECISION: %u\n", DEFAULT_DOUBLE_TO_STRING_PRECISION );
    printf( "  REFACTORIZATION_THRESHOLD: %u\n", REFACTORIZATION_THRESHOLD );
    printf( "  STATISTICS_PRINTING_FREQUENCY: %u\n", STATISTICS_PRINTING_FREQUENCY );
    printf( "  NUMERICAL_STABILITY_CONSTANT: %lf\n", NUMERICAL_STABILITY_CONSTANT );
    printf( "****************************\n" );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
