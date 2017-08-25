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
const double GlobalConfiguration::BOUND_COMPARISON_TOLERANCE = 0.001;
const double GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE = 0.000000001;
const bool GlobalConfiguration::PREPROCESS_INPUT_QUERY = true;
const unsigned GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY = 10;
const double GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD = 0.0001;
const unsigned GlobalConfiguration::MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS = 5;
const unsigned GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD = 20;

const unsigned GlobalConfiguration::PSE_ITERATIONS_BEFORE_RESET = 1000;
const double GlobalConfiguration::PSE_GAMMA_ERROR_THRESHOLD = 0.001;

// Logging
const bool GlobalConfiguration::ENGINE_LOGGING = false;
const bool GlobalConfiguration::TABLEAU_LOGGING = false;
const bool GlobalConfiguration::SMT_CORE_LOGGING = false;
const bool GlobalConfiguration::DANTZIGS_RULE_LOGGING = false;
const bool GlobalConfiguration::BASIS_FACTORIZATION_LOGGING = false;
const bool GlobalConfiguration::PROJECTED_STEEPEST_EDGE_LOGGING = false;

void GlobalConfiguration::print()
{
    printf( "****************************\n" );
    printf( "*** Global Configuraiton ***\n" );
    printf( "****************************\n" );
    printf( "  DEFAULT_EPSILON_FOR_COMPARISONS: %.15lf\n", DEFAULT_EPSILON_FOR_COMPARISONS );
    printf( "  DEFAULT_DOUBLE_TO_STRING_PRECISION: %u\n", DEFAULT_DOUBLE_TO_STRING_PRECISION );
    printf( "  REFACTORIZATION_THRESHOLD: %u\n", REFACTORIZATION_THRESHOLD );
    printf( "  STATISTICS_PRINTING_FREQUENCY: %u\n", STATISTICS_PRINTING_FREQUENCY );
    printf( "  BOUND_COMPARISON_TOLERANCE: %.15lf\n", BOUND_COMPARISON_TOLERANCE );
    printf( "  PIVOT_CHANGE_COLUMN_TOLERANCE: %.15lf\n", PIVOT_CHANGE_COLUMN_TOLERANCE );
    printf( "  PREPROCESS_INPUT_QUERY: %s\n", PREPROCESS_INPUT_QUERY ? "Yes" : "No" );
    printf( "  DEGRADATION_CHECKING_FREQUENCY: %u\n", DEGRADATION_CHECKING_FREQUENCY );
    printf( "  ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD: %.15lf\n", ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD );
    printf( "  MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS: %u\n", MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS );
    printf( "  CONSTRAINT_VIOLATION_THRESHOLD: %u\n", CONSTRAINT_VIOLATION_THRESHOLD );

    printf( "  PSE_ITERATIONS_BEFORE_RESET: %u\n", PSE_ITERATIONS_BEFORE_RESET );
    printf( "  PSE_GAMMA_ERROR_THRESHOLD: %.15lf\n", PSE_GAMMA_ERROR_THRESHOLD );
    printf( "****************************\n" );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
