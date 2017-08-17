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

    static const double DEFAULT_EPSILON_FOR_COMPARISONS;
    static const unsigned DEFAULT_DOUBLE_TO_STRING_PRECISION;
	static const unsigned REFACTORIZATION_THRESHOLD;
    static const unsigned STATISTICS_PRINTING_FREQUENCY;
    static const double NUMERICAL_STABILITY_CONSTANT;
    static const double BOUND_COMPARISON_TOLERANCE;
	static const bool PREPROCESS_INPUT_QUERY;
    static const unsigned DEGRADATION_CHECKING_FREQUENCY;

    static const unsigned PSE_ITERATIONS_BEFORE_RESET;
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
