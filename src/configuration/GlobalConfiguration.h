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

#ifndef __GlobalConfiguration_h__
#define __GlobalConfiguration_h__

class GlobalConfiguration
{
public:
    static void print();

    // The default epsilon used for comparing doubles
    static const double DEFAULT_EPSILON_FOR_COMPARISONS;

    // The precision level when convering doubles to strings
    static const unsigned DEFAULT_DOUBLE_TO_STRING_PRECISION;

    // The number of accumualted eta matrices, after which the basis will be refactorized
	static const unsigned REFACTORIZATION_THRESHOLD;

    // How often should the main loop print statistics?
    static const unsigned STATISTICS_PRINTING_FREQUENCY;

    // Tolerance when checking whether the value computed for a basic variable is out of bounds
    static const double BOUND_COMPARISON_TOLERANCE;

    // Tolerance when checking whether a basic variable depends on a non-basic variable, by looking
    // at the change column, as part of a pivot operation.
    static const double PIVOT_CHANGE_COLUMN_TOLERANCE;

    // Toggle query-preprocessing on/off
	static const bool PREPROCESS_INPUT_QUERY;

    // How often should the main loop check the current degradation?
    static const unsigned DEGRADATION_CHECKING_FREQUENCY;

    // If a pivot element in a simplex element is smaller than this threshold, the engine will attempt
    // to pick another element.
    static const double ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD;

    // How many potential pivots should the engine inspect (at most) in every simplex iteration?
    static const unsigned MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS;

    // How often should projected steepest edge reset the reference space?
    static const unsigned PSE_ITERATIONS_BEFORE_RESET;

    // An error threshold which, when crossed, causes projected steepest edge to reset the reference space
    static const double PSE_GAMMA_ERROR_THRESHOLD;


    // Logging
    static const bool ENGINE_LOGGING;
    static const bool TABLEAU_LOGGING;
    static const bool SMT_CORE_LOGGING;
    static const bool DANTZIGS_RULE_LOGGING;
    static const bool BASIS_FACTORIZATION_LOGGING;
    static const bool PROJECTED_STEEPEST_EDGE_LOGGING;
};

#endif // __GlobalConfiguration_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
