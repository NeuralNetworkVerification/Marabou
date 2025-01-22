/*********************                                                        */
/*! \file Options.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Options.h"

#include "ConfigurationError.h"
#include "Debug.h"
#include "GlobalConfiguration.h"

Options *Options::get()
{
    static Options singleton;
    return &singleton;
}

Options::Options()
    : _optionParser( &_boolOptions, &_intOptions, &_floatOptions, &_stringOptions )
{
    initializeDefaultValues();
    _optionParser.initialize();
}

Options::Options( const Options & )
{
    // This constructor should never be called
    ASSERT( false );
}

void Options::initializeDefaultValues()
{
    /*
      Bool options
    */
    _boolOptions[DNC_MODE] = false;
    _boolOptions[PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS] = false;
    _boolOptions[RESTORE_TREE_STATES] = false;
    _boolOptions[DUMP_BOUNDS] = false;
    _boolOptions[DUMP_TOPOLOGY] = false;
    _boolOptions[SOLVE_WITH_MILP] = false;
    _boolOptions[PERFORM_LP_TIGHTENING_AFTER_SPLIT] = false;
    _boolOptions[PARALLEL_DEEPSOI] = false;
    _boolOptions[EXPORT_ASSIGNMENT] = false;
    _boolOptions[DEBUG_ASSIGNMENT] = false;
    _boolOptions[PRODUCE_PROOFS] = false;
    _boolOptions[DO_NOT_MERGE_CONSECUTIVE_WEIGHTED_SUM_LAYERS] = false;

    /*
      Int options
    */
    _intOptions[NUM_WORKERS] = 1;
    _intOptions[NUM_INITIAL_DIVIDES] = 0;
    _intOptions[NUM_ONLINE_DIVIDES] = 2;
    _intOptions[INITIAL_TIMEOUT] = 5;
    _intOptions[VERBOSITY] = 2;
    _intOptions[TIMEOUT] = 0;
    _intOptions[CONSTRAINT_VIOLATION_THRESHOLD] = 20;
    _intOptions[DEEP_SOI_REJECTION_THRESHOLD] = 2;
    _intOptions[NUMBER_OF_SIMULATIONS] = 100;
    _intOptions[SEED] = 1;
    _intOptions[NUM_BLAS_THREADS] = 1;
    _intOptions[NUM_CONSTRAINTS_TO_REFINE_INC_LIN] = 30;

    /*
      Float options
    */
    _floatOptions[TIMEOUT_FACTOR] = 1.5;
    _floatOptions[MILP_SOLVER_TIMEOUT] = 1.0;
    _floatOptions[PREPROCESSOR_BOUND_TOLERANCE] =
        GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS;
    _floatOptions[PROBABILITY_DENSITY_PARAMETER] = 10;
    _floatOptions[REFINEMENT_SCALING_FACTOR_INC_LIN] = 2;

    /*
      String options
    */
    _stringOptions[INPUT_FILE_PATH] = "";
    _stringOptions[PROPERTY_FILE_PATH] = "";
    _stringOptions[INPUT_QUERY_FILE_PATH] = "";
    _stringOptions[SUMMARY_FILE] = "";
    _stringOptions[SPLITTING_STRATEGY] = "auto";
    _stringOptions[SNC_SPLITTING_STRATEGY] = "auto";
    _stringOptions[SYMBOLIC_BOUND_TIGHTENING_TYPE] = "deeppoly";
    _stringOptions[MILP_SOLVER_BOUND_TIGHTENING_TYPE] = "none";
    _stringOptions[QUERY_DUMP_FILE] = "";
    _stringOptions[IMPORT_ASSIGNMENT_FILE_PATH] = "assignment.txt";
    _stringOptions[EXPORT_ASSIGNMENT_FILE_PATH] = "assignment.txt";
    _stringOptions[SOI_SEARCH_STRATEGY] = "mcmc";
    _stringOptions[SOI_INITIALIZATION_STRATEGY] = "input-assignment";
    _stringOptions[LP_SOLVER] = gurobiEnabled() ? "gurobi" : "native";
    _stringOptions[SOFTMAX_BOUND_TYPE] = "lse";
}

void Options::parseOptions( int argc, char **argv )
{
    _optionParser.parse( argc, argv );
}

void Options::printHelpMessage() const
{
    _optionParser.printHelpMessage();
}

bool Options::getBool( unsigned option ) const
{
    return _boolOptions.get( option );
}

int Options::getInt( unsigned option ) const
{
    return _intOptions.get( option );
}

float Options::getFloat( unsigned option ) const
{
    return _floatOptions.get( option );
}

String Options::getString( unsigned option ) const
{
    return String( _stringOptions.get( option ) );
}

void Options::setBool( unsigned option, bool value )
{
    _boolOptions[option] = value;
}

void Options::setInt( unsigned option, int value )
{
    _intOptions[option] = value;
}

void Options::setFloat( unsigned option, float value )
{
    _floatOptions[option] = value;
}

void Options::setString( unsigned option, std::string value )
{
    _stringOptions[option] = value;
}

DivideStrategy Options::getDivideStrategy() const
{
    String strategyString = String( _stringOptions.get( Options::SPLITTING_STRATEGY ) );
    if ( strategyString == "polarity" )
        return DivideStrategy::Polarity;
    else if ( strategyString == "earliest-relu" )
        return DivideStrategy::EarliestReLU;
    else if ( strategyString == "relu-violation" )
        return DivideStrategy::ReLUViolation;
    else if ( strategyString == "largest-interval" )
        return DivideStrategy::LargestInterval;
    else if ( strategyString == "pseudo-impact" )
        return DivideStrategy::PseudoImpact;
    else if ( strategyString == "babsr" )
        return DivideStrategy::BaBSR;
    else
        return DivideStrategy::Auto;
}

SnCDivideStrategy Options::getSnCDivideStrategy() const
{
    String strategyString = String( _stringOptions.get( Options::SNC_SPLITTING_STRATEGY ) );
    if ( strategyString == "polarity" )
        return SnCDivideStrategy::Polarity;
    else if ( strategyString == "largest-interval" )
        return SnCDivideStrategy::LargestInterval;
    else
        return SnCDivideStrategy::Auto;
}

SymbolicBoundTighteningType Options::getSymbolicBoundTighteningType() const
{
    String strategyString = String( _stringOptions.get( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE ) );
    if ( strategyString == "sbt" )
        return SymbolicBoundTighteningType::SYMBOLIC_BOUND_TIGHTENING;
    else if ( strategyString == "deeppoly" )
        return SymbolicBoundTighteningType::DEEP_POLY;
    else if ( strategyString == "none" )
        return SymbolicBoundTighteningType::NONE;
    else
        return SymbolicBoundTighteningType::DEEP_POLY;
}

MILPSolverBoundTighteningType Options::getMILPSolverBoundTighteningType() const
{
    if ( gurobiEnabled() )
    {
        String strategyString =
            String( _stringOptions.get( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE ) );
        if ( strategyString == "lp" )
            return MILPSolverBoundTighteningType::LP_RELAXATION;
        else if ( strategyString == "lp-inc" )
            return MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL;
        if ( strategyString == "backward-once" )
            return MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_ONCE;
        if ( strategyString == "backward-converge" )
            return MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE;
        else if ( strategyString == "milp" )
            return MILPSolverBoundTighteningType::MILP_ENCODING;
        else if ( strategyString == "milp-inc" )
            return MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL;
        else if ( strategyString == "iter-prop" )
            return MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION;
        else if ( strategyString == "none" )
            return MILPSolverBoundTighteningType::NONE;
        else
            return MILPSolverBoundTighteningType::LP_RELAXATION;
    }
    else
    {
        return MILPSolverBoundTighteningType::NONE;
    }
}

SoISearchStrategy Options::getSoISearchStrategy() const
{
    String strategyString = String( _stringOptions.get( Options::SOI_SEARCH_STRATEGY ) );
    if ( strategyString == "mcmc" )
        return SoISearchStrategy::MCMC;
    else if ( strategyString == "walksat" )
        return SoISearchStrategy::WALKSAT;
    else
        return SoISearchStrategy::MCMC;
}

SoIInitializationStrategy Options::getSoIInitializationStrategy() const
{
    String strategyString = String( _stringOptions.get( Options::SOI_INITIALIZATION_STRATEGY ) );
    if ( strategyString == "input-assignment" )
        return SoIInitializationStrategy::INPUT_ASSIGNMENT;
    if ( strategyString == "current-assignment" )
        return SoIInitializationStrategy::CURRENT_ASSIGNMENT;
    else
        return SoIInitializationStrategy::INPUT_ASSIGNMENT;
}

LPSolverType Options::getLPSolverType() const
{
    String solverString = String( _stringOptions.get( Options::LP_SOLVER ) );
    if ( solverString == "native" )
        return LPSolverType::NATIVE;
    else if ( _boolOptions.get( Options::PRODUCE_PROOFS ) )
    {
        printf( "Proof-producing mode on, using native LP engine..." );
        return LPSolverType::NATIVE;
    }
    else if ( solverString == "gurobi" )
        return LPSolverType::GUROBI;
    else
        return gurobiEnabled() ? LPSolverType::GUROBI : LPSolverType::NATIVE;
}

SoftmaxBoundType Options::getSoftmaxBoundType() const
{
    String boundType = String( _stringOptions.get( Options::SOFTMAX_BOUND_TYPE ) );
    if ( boundType == "er" )
        return SoftmaxBoundType::EXPONENTIAL_RECIPROCAL_DECOMPOSITION;
    else if ( boundType == "lse" )
        return SoftmaxBoundType::LOG_SUM_EXP_DECOMPOSITION;
    else
    {
        return SoftmaxBoundType::LOG_SUM_EXP_DECOMPOSITION;
    }
}
