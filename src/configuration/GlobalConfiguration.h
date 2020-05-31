/*********************                                                        */
/*! \file GlobalConfiguration.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __GlobalConfiguration_h__
#define __GlobalConfiguration_h__

class GlobalConfiguration
{
public:
    static void print();

    static const bool USE_POLARITY_BASED_DIRECTION_HEURISTICS;

    // The default epsilon used for comparing doubles
    static const double DEFAULT_EPSILON_FOR_COMPARISONS;

    // The precision level when convering doubles to strings
    static const unsigned DEFAULT_DOUBLE_TO_STRING_PRECISION;

    // How often should the main loop print statistics?
    static const unsigned STATISTICS_PRINTING_FREQUENCY;

    // Tolerance when checking whether the value computed for a basic variable is out of bounds
    static const double BOUND_COMPARISON_ADDITIVE_TOLERANCE;
    static const double BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE;

    // Tolerance when checking whether a basic variable depends on a non-basic variable, by looking
    // at the change column, as part of a pivot operation.
    static const double PIVOT_CHANGE_COLUMN_TOLERANCE;

    // Tolerance for the difference when computing the pivot entry by column and by row
    static const double PIVOT_ROW_AND_COLUMN_TOLERANCE;

    // Tolerance when checking whether a non-basic variable is eligible for being selected as the
    // entering variable, by its reduced cost
    static const double ENTRY_ELIGIBILITY_TOLERANCE;

    // Ratio test tolerance constants
    static const double RATIO_CONSTRAINT_ADDITIVE_TOLERANCE;
    static const double RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;
    static const double HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE;
    static const double HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;

    // Cost function tolerance constants
    static const double BASIC_COSTS_ADDITIVE_TOLERANCE;
    static const double BASIC_COSTS_MULTIPLICATIVE_TOLERANCE;

    // Sparse ForrestTomlin diagonal element tolerance constant
    static const double SPARSE_FORREST_TOMLIN_DIAGONAL_ELEMENT_TOLERANCE;

    // Toggle use of Harris' two-pass ratio test for selecting the leaving variable
    static const bool USE_HARRIS_RATIO_TEST;

    // Toggle query-preprocessing on/off.
	static const bool PREPROCESS_INPUT_QUERY;

    // Assuming the preprocessor is on, toggle whether or not it will attempt to perform variable
    // elimination.
    static const bool PREPROCESSOR_ELIMINATE_VARIABLES;

    // Assuming the preprocessor is on, toggle whether or not PL constraints will be called upon
    // to add auxiliary variables and equations.
    static const bool PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS;

    // If the difference between a variable's lower and upper bounds is smaller than this
    // threshold, the preprocessor will treat it as fixed.
    static const double PREPROCESSOR_ALMOST_FIXED_THRESHOLD;

    // Try to set the initial tableau assignment to an assignment that is legal with
    // respect to the input network.
    static const bool WARM_START;

    // The maximal number of iterations without new tree states being visited, before
    // the engine performs a precision restoration.
    static const unsigned MAX_ITERATIONS_WITHOUT_PROGRESS;

    // How often should the main loop check the current degradation?
    static const unsigned DEGRADATION_CHECKING_FREQUENCY;

    // The threshold of degradation above which restoration is required
    static const double DEGRADATION_THRESHOLD;

    // If a pivot element in a simplex iteration is smaller than this threshold, the engine will attempt
    // to pick another element.
    static const double ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD;

    // If true, column-merging equations are given special treatment and cause columns in the tableau
    // to be merged (instead of a new row added).
    static const bool USE_COLUMN_MERGING_EQUATIONS;

    // If a pivot element in a Gaussian elimination iteration is smaller than this threshold times
    // the largest element in the column, the elimination engine will attempt to pick another pivot.
    static const double GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD;

    // How many potential pivots should the engine inspect (at most) in every simplex iteration?
    static const unsigned MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS;

    // The number of violations of a constraints after which the SMT core will initiate a case split
    static const unsigned CONSTRAINT_VIOLATION_THRESHOLD;

    static const unsigned SPLITTING_HEURISTICS;

    // How often should we perform full bound tightening, on the entire contraints matrix A.
    static const unsigned BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY;

    // When the row bound tightener is asked to run until saturation, it can enter an infinite loop
    // due to tiny increments in bounds. This number limits the number of iterations it can perform.
    static const unsigned ROW_BOUND_TIGHTENER_SATURATION_ITERATIONS;

    // If the cost function error exceeds this threshold, it is recomputed
    static const double COST_FUNCTION_ERROR_THRESHOLD;

    // How often should projected steepest edge reset the reference space?
    static const unsigned PSE_ITERATIONS_BEFORE_RESET;

    // An error threshold which, when crossed, causes projected steepest edge to reset the reference space
    static const double PSE_GAMMA_ERROR_THRESHOLD;

    // PSE's Gamma function's update tolerance
    static const double PSE_GAMMA_UPDATE_TOLERANCE;

    // The tolerance for checking whether f = Relu( b )
    static const double RELU_CONSTRAINT_COMPARISON_TOLERANCE;

    // The tolerance for checking whether f = Abs( b )
    static const double ABS_CONSTRAINT_COMPARISON_TOLERANCE;

    // Should the initial basis be comprised only of auxiliary (row) variables?
    static const bool ONLY_AUX_INITIAL_BASIS;

    /*
      Explicit (Reluplex-style) bound tightening options
    */

    enum ExplicitBasisBoundTighteningType {
        // Compute the inverse basis matrix and use it
        COMPUTE_INVERTED_BASIS_MATRIX = 0,
        // Use the inverted basis matrix without using it, via transformations
        USE_IMPLICIT_INVERTED_BASIS_MATRIX = 1,
    };

    // When doing bound tightening using the explicit basis matrix, should the basis matrix be inverted?
    static const ExplicitBasisBoundTighteningType EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE;

    // When doing explicit bound tightening, should we repeat until saturation?
    static const bool EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION;

    /*
      Symbolic bound tightening options
    */

    // Whether symbolic bound tightening should be used or not
    static const bool USE_SYMBOLIC_BOUND_TIGHTENING;

    // Symbolic tightening rounding constant
    static const double SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;

    /*
      Constraint fixing heuristics
    */

    // When a PL constraint proposes a fix that affects multiple variables, should it first query
    // for any relevant linear connections between the variables?
    static const bool USE_SMART_FIX;

    // A heuristic for selecting which of the broken PL constraints will be repaired next. In this case,
    // the one that has been repaired the least number of times so far.
    static const bool USE_LEAST_FIX;

    /*
      Basis factorization options
    */

    // The number of accumualted eta matrices, after which the basis will be refactorized
	static const unsigned REFACTORIZATION_THRESHOLD;

    // The kind of basis factorization algorithm in use
    enum BasisFactorizationType {
        LU_FACTORIZATION,
        SPARSE_LU_FACTORIZATION,
        FORREST_TOMLIN_FACTORIZATION,
        SPARSE_FORREST_TOMLIN_FACTORIZATION,
    };
    static const BasisFactorizationType BASIS_FACTORIZATION_TYPE;

    /*
      Logging options
    */
    static const bool DNC_MANAGER_LOGGING;
    static const bool ENGINE_LOGGING;
    static const bool TABLEAU_LOGGING;
    static const bool SMT_CORE_LOGGING;
    static const bool DANTZIGS_RULE_LOGGING;
    static const bool BASIS_FACTORIZATION_LOGGING;
    static const bool PREPROCESSOR_LOGGING;
    static const bool PROJECTED_STEEPEST_EDGE_LOGGING;
    static const bool GAUSSIAN_ELIMINATION_LOGGING;
    static const bool QUERY_LOADER_LOGGING;
    static const bool SYMBOLIC_BOUND_TIGHTENER_LOGGING;
    static const bool NETWORK_LEVEL_REASONER_LOGGING;
};

#endif // __GlobalConfiguration_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
