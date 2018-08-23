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

#include "GlobalConfiguration.h"
#include "MString.h"
#include <cstdio>

const double GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS = 0.0000000001;
const unsigned GlobalConfiguration::DEFAULT_DOUBLE_TO_STRING_PRECISION = 10;
const unsigned GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY = 1000;
const double GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE = 0.0000001;
const double GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE = 0.001 * 0.0000001;
const double GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE = 0.000000001;
const double GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE = 0.00000001;
const double GlobalConfiguration::RATIO_CONSTRAINT_ADDITIVE_TOLERANCE = 0.0000001 * 0.3;
const double GlobalConfiguration::RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE = 0.001 * 0.0000001 * 0.3;
const double GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE = 0.0000001 * 0.5;
const double GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE = 0.001 * 0.0000001 * 0.5;
const double GlobalConfiguration::BASIC_COSTS_ADDITIVE_TOLERANCE = 0.0000001;
const double GlobalConfiguration::BASIC_COSTS_MULTIPLICATIVE_TOLERANCE = 0.001 * 0.0000001;
const unsigned GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY = 100;
const double GlobalConfiguration::DEGRADATION_THRESHOLD = 0.1;
const double GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD = 0.0001;
const bool GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS = true;
const double GlobalConfiguration::GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD = 0.1;
const unsigned GlobalConfiguration::MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS = 5;
const unsigned GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD = 20;
const unsigned GlobalConfiguration::BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY = 100;
const double GlobalConfiguration::COST_FUNCTION_ERROR_THRESHOLD = 0.0000000001;

const bool GlobalConfiguration::USE_HARRIS_RATIO_TEST = true;

const bool GlobalConfiguration::PREPROCESS_INPUT_QUERY = true;
const bool GlobalConfiguration::PREPROCESSOR_ELIMINATE_VARIABLES = true;
const bool GlobalConfiguration::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS = true;

const unsigned GlobalConfiguration::PSE_ITERATIONS_BEFORE_RESET = 1000;
const double GlobalConfiguration::PSE_GAMMA_ERROR_THRESHOLD = 0.001;
const double GlobalConfiguration::PSE_GAMMA_UPDATE_TOLERANCE = 0.000000001;


const double GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE = 0.001;

const GlobalConfiguration::ExplicitBasisBoundTighteningType GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE =
    GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX;
const bool GlobalConfiguration::EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION = false;

const unsigned GlobalConfiguration::REFACTORIZATION_THRESHOLD = 100;
const GlobalConfiguration::BasisFactorizationType GlobalConfiguration::BASIS_FACTORIZATION_TYPE =
    GlobalConfiguration::SPARSE_FORREST_TOMLIN_FACTORIZATION;

// Logging
const bool GlobalConfiguration::ENGINE_LOGGING = false;
const bool GlobalConfiguration::TABLEAU_LOGGING = false;
const bool GlobalConfiguration::SMT_CORE_LOGGING = false;
const bool GlobalConfiguration::DANTZIGS_RULE_LOGGING = false;
const bool GlobalConfiguration::BASIS_FACTORIZATION_LOGGING = false;
const bool GlobalConfiguration::PROJECTED_STEEPEST_EDGE_LOGGING = false;
const bool GlobalConfiguration::GAUSSIAN_ELIMINATION_LOGGING = false;

void GlobalConfiguration::print()
{
    printf( "****************************\n" );
    printf( "*** Global Configuraiton ***\n" );
    printf( "****************************\n" );
    printf( "  DEFAULT_EPSILON_FOR_COMPARISONS: %.15lf\n", DEFAULT_EPSILON_FOR_COMPARISONS );
    printf( "  DEFAULT_DOUBLE_TO_STRING_PRECISION: %u\n", DEFAULT_DOUBLE_TO_STRING_PRECISION );
    printf( "  STATISTICS_PRINTING_FREQUENCY: %u\n", STATISTICS_PRINTING_FREQUENCY );
    printf( "  BOUND_COMPARISON_ADDITIVE_TOLERANCE: %.15lf\n", BOUND_COMPARISON_ADDITIVE_TOLERANCE );
    printf( "  BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE: %.15lf\n", BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE );
    printf( "  PIVOT_CHANGE_COLUMN_TOLERANCE: %.15lf\n", PIVOT_CHANGE_COLUMN_TOLERANCE );
    printf( "  RATIO_CONSTRAINT_ADDITIVE_TOLERANCE: %.15lf\n", RATIO_CONSTRAINT_ADDITIVE_TOLERANCE );
    printf( "  RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE: %.15lf\n", RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE );
    printf( "  BASIC_COSTS_ADDITIVE_TOLERANCE: %.15lf\n", BASIC_COSTS_ADDITIVE_TOLERANCE );
    printf( "  BASIC_COSTS_MULTIPLICATIVE_TOLERANCE: %.15lf\n", BASIC_COSTS_MULTIPLICATIVE_TOLERANCE );
    printf( "  DEGRADATION_CHECKING_FREQUENCY: %u\n", DEGRADATION_CHECKING_FREQUENCY );
    printf( "  DEGRADATION_THRESHOLD: %.15lf\n", DEGRADATION_THRESHOLD );
    printf( "  ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD: %.15lf\n", ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD );
    printf( "  USE_COLUMN_MERGING_EQUATIONS: %s\n", USE_COLUMN_MERGING_EQUATIONS ? "Yes" : "No" );
    printf( "  GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD: %.15lf\n", GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD );
    printf( "  MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS: %u\n", MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS );
    printf( "  CONSTRAINT_VIOLATION_THRESHOLD: %u\n", CONSTRAINT_VIOLATION_THRESHOLD );
    printf( "  BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY: %u\n",
            BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY );
    printf( "  COST_FUNCTION_ERROR_THRESHOLD: %.15lf\n", COST_FUNCTION_ERROR_THRESHOLD );
    printf( "  USE_HARRIS_RATIO_TEST: %s\n", USE_HARRIS_RATIO_TEST ? "Yes" : "No" );

    printf( "  PREPROCESS_INPUT_QUERY: %s\n", PREPROCESS_INPUT_QUERY ? "Yes" : "No" );
    printf( "  PREPROCESSOR_ELIMINATE_VARIABLES: %s\n", PREPROCESSOR_ELIMINATE_VARIABLES ? "Yes" : "No" );
    printf( "  PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS: %s\n",
            PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS ? "Yes" : "No" );
    printf( "  PSE_ITERATIONS_BEFORE_RESET: %u\n", PSE_ITERATIONS_BEFORE_RESET );
    printf( "  PSE_GAMMA_ERROR_THRESHOLD: %.15lf\n", PSE_GAMMA_ERROR_THRESHOLD );
    printf( "  RELU_CONSTRAINT_COMPARISON_TOLERANCE: %.15lf\n", RELU_CONSTRAINT_COMPARISON_TOLERANCE );

    String basisBoundTighteningType;
    switch ( EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE )
    {
    case COMPUTE_INVERTED_BASIS_MATRIX:
        basisBoundTighteningType = "Compute inverted basis matrix";
        break;

    case USE_IMPLICIT_INVERTED_BASIS_MATRIX:
        basisBoundTighteningType = "Use implicit inverted basis matrix";
        break;

    default:
        basisBoundTighteningType = "Unknown";
        break;
    }

    printf( "  EXPLICIT_BASIS_BOUND_TIGHTENING_INVERT_BASIS: %s\n", basisBoundTighteningType.ascii() );
    printf( "  EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION: %s\n",
            EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION ? "Yes" : "No" );
    printf( "  REFACTORIZATION_THRESHOLD: %u\n", REFACTORIZATION_THRESHOLD );

    String basisFactorizationType;
    if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE == GlobalConfiguration::LU_FACTORIZATION )
        basisFactorizationType = "LU_FACTORIZATION";
    else if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
              GlobalConfiguration::SPARSE_LU_FACTORIZATION )
        basisFactorizationType = "SPARSE_LU_FACTORIZATION";
    else if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
              GlobalConfiguration::FORREST_TOMLIN_FACTORIZATION )
        basisFactorizationType = "FORREST_TOMLIN_FACTORIZATION";
    else
        basisFactorizationType = "Unknown";

    printf( "  BASIS_FACTORIZATION_TYPE: %s\n", basisFactorizationType.ascii() );
    printf( "****************************\n" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
