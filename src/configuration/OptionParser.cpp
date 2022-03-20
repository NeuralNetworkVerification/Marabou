/*********************                                                        */
/*! \file OptionParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Debug.h"
#include "OptionParser.h"
#include "Options.h"

OptionParser::OptionParser()
{
    // This constructor should not be called
    ASSERT( false );
}

OptionParser::OptionParser( Map<unsigned, bool> *boolOptions,
                            Map<unsigned, int> *intOptions,
                            Map<unsigned, float> *floatOptions,
                            Map<unsigned, std::string> *stringOptions )
    : _positional( "" )
    , _common( "Common options" )
    , _other( "Less common options" )
    , _expert( "More advanced internal options" )
    , _boolOptions( boolOptions )
    , _intOptions( intOptions )
    , _floatOptions( floatOptions )
    , _stringOptions( stringOptions )
{
}

void OptionParser::initialize()
{
    _positional.add_options()
        ( "input",
          boost::program_options::value<std::string>( &(*_stringOptions)[Options::INPUT_FILE_PATH] )->default_value( (*_stringOptions)[Options::INPUT_FILE_PATH] ),
          "Neural netowrk file." )
        ( "property",
          boost::program_options::value<std::string>( &(*_stringOptions)[Options::PROPERTY_FILE_PATH] )->default_value( (*_stringOptions)[Options::PROPERTY_FILE_PATH] ),
          "Property file." )
        ;

    // Most common options
    _common.add_options()
        ( "help",
          boost::program_options::bool_switch( &(*_boolOptions)[Options::HELP] )->default_value( (*_boolOptions)[Options::HELP] ),
          "Prints the help message.")
        ( "version",
          boost::program_options::bool_switch( &(*_boolOptions)[Options::VERSION] )->default_value( (*_boolOptions)[Options::VERSION] ),
          "Prints the version number.")
        ( "input-query",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::INPUT_QUERY_FILE_PATH]) )->default_value( (*_stringOptions)[Options::INPUT_QUERY_FILE_PATH] ),
          "Input Query file. When specified, Marabou will solve this instead of the network and property pair." )
        ( "num-workers",
          boost::program_options::value<int>( &(*_intOptions)[Options::NUM_WORKERS] )->default_value( (*_intOptions)[Options::NUM_WORKERS] ),
          "Number of threads to use." )
        ( "timeout",
          boost::program_options::value<int>( &(*_intOptions)[Options::TIMEOUT] )->default_value( (*_intOptions)[Options::TIMEOUT] ),
          "Global timeout in seconds. 0 means no timeout." )
#ifdef ENABLE_GUROBI
        ( "milp",
          boost::program_options::bool_switch( &(*_boolOptions)[Options::SOLVE_WITH_MILP] )->default_value( (*_boolOptions)[Options::SOLVE_WITH_MILP] ),
          "Solve the input query with a MILP encoding in Gruobi." )
#endif
        ;

    // Less common options
    _other.add_options()
        ( "verbosity",
          boost::program_options::value<int>( &((*_intOptions)[Options::VERBOSITY]) )->default_value( (*_intOptions)[Options::VERBOSITY] ),
          "Verbosity of engine::solve(). 0: does not print anything, 1: print"
          "out statistics in the beginning and end, 2: print out statistics during solving." )
        ( "snc",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::DNC_MODE]) )->default_value( (*_boolOptions)[Options::DNC_MODE] ),
          "Use the split-and-conquer solving mode." )
        ( "seed",
          boost::program_options::value<int>( &((*_intOptions)[Options::SEED]) )->default_value( (*_intOptions)[Options::SEED] ),
          "The random seed." )
        ( "dump-bounds",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::DUMP_BOUNDS]) )->default_value( (*_boolOptions)[Options::DUMP_BOUNDS] ),
          "Dump the bounds after preprocessing." )
        ( "query-dump-file",
          boost::program_options::value<std::string>( &(*_stringOptions)[Options::QUERY_DUMP_FILE] )->default_value( (*_stringOptions)[Options::QUERY_DUMP_FILE] ),
          "Dump the verification query in Marabou's input query format." )
        ( "summary-file",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SUMMARY_FILE]) )->default_value( (*_stringOptions)[Options::SUMMARY_FILE] ),
          "Produce a summary file of the run." )
#ifdef ENABLE_GUROBI
#endif // ENABLE_GUROBI
        ;

    _expert.add_options()
        ( "tightening-strategy",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SYMBOLIC_BOUND_TIGHTENING_TYPE]) )->default_value( (*_stringOptions)[Options::SYMBOLIC_BOUND_TIGHTENING_TYPE] ),
          "type of bound tightening technique to use: sbt/deeppoly/none." )
        ( "branch",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SPLITTING_STRATEGY]) )->default_value( (*_stringOptions)[Options::SPLITTING_STRATEGY] ),
          "The branching strategy (earliest-relu/pseudo-impact/largest-interval/relu-violation/polarity)."
          " pseudo-impact is specific to the DeepSoI (default) procedure and relu-violation is specific to the Reluplex procedure.\n" )
        ( "soi-split-threshold",
          boost::program_options::value<int>( &((*_intOptions)[Options::DEEP_SOI_REJECTION_THRESHOLD]) )->default_value( (*_intOptions)[Options::DEEP_SOI_REJECTION_THRESHOLD] ),
          "(DeepSoI) Max number of rejected phase pattern proposal before splitting." )
        ( "soi-search-strategy",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SOI_SEARCH_STRATEGY]) )->default_value( (*_stringOptions)[Options::SOI_SEARCH_STRATEGY] ),
          "(DeepSoI) Strategy for stochastically minimizing the soi: mcmc/walksat." )
        ( "soi-init-strategy",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SOI_INITIALIZATION_STRATEGY]) )->default_value( (*_stringOptions)[Options::SOI_INITIALIZATION_STRATEGY] ),
          "(DeepSoI) Strategy for initialize the soi function: input-assignment/current-assignment. default: input-assignment." )
        ( "mcmc-beta",
          boost::program_options::value<float>( &((*_floatOptions)[Options::PROBABILITY_DENSITY_PARAMETER]) )->default_value( (*_floatOptions)[Options::PROBABILITY_DENSITY_PARAMETER] ),
          "(DeepSoI) The beta parameter in MCMC search.\n" )
        ( "split-strategy",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SNC_SPLITTING_STRATEGY]) )->default_value( (*_stringOptions)[Options::SNC_SPLITTING_STRATEGY] ),
          "(SnC) The splitting strategy." )
        ( "initial-divides",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_INITIAL_DIVIDES]) )->default_value( (*_intOptions)[Options::NUM_INITIAL_DIVIDES] ),
          "(SnC) Number of times to initially bisect the input region." )
        ( "initial-timeout",
          boost::program_options::value<int>( &((*_intOptions)[Options::INITIAL_TIMEOUT]) )->default_value( (*_intOptions)[Options::INITIAL_TIMEOUT] ),
          "(SnC) The initial timeout." )
        ( "num-online-divides",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_ONLINE_DIVIDES]) )->default_value( (*_intOptions)[Options::NUM_ONLINE_DIVIDES] ),
          "(SnC) Number of times to further bisect a sub-region when a timeout occurs." )
        ( "timeout-factor",
          boost::program_options::value<float>( &((*_floatOptions)[Options::TIMEOUT_FACTOR]) )->default_value( (*_floatOptions)[Options::TIMEOUT_FACTOR] ),
          "(SnC) The timeout factor." )
        ( "restore-tree-states",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::RESTORE_TREE_STATES]) )->default_value( (*_boolOptions)[Options::RESTORE_TREE_STATES] ),
          "(SnC) Restore tree states in SnC mode.\n" )
        ( "blas-threads",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_BLAS_THREADS]) )->default_value( (*_intOptions)[Options::NUM_BLAS_THREADS] ),
          "Number of threads to use for matrix multiplication with OpenBLAS." )
        ( "reluplex-split-threshold",
          boost::program_options::value<int>( &((*_intOptions)[Options::CONSTRAINT_VIOLATION_THRESHOLD]) )->default_value( (*_intOptions)[Options::CONSTRAINT_VIOLATION_THRESHOLD] ),
          "Max number of tries to repair a relu before splitting when the Reluplex procedure is used." )
        ( "preprocessor-bound-tolerance",
          boost::program_options::value<float>( &((*_floatOptions)[Options::PREPROCESSOR_BOUND_TOLERANCE]) )->default_value( (*_floatOptions)[Options::PREPROCESSOR_BOUND_TOLERANCE] ),
          "epsilon for preprocessor bound tightening comparisons." )
        ( "no-parallel-deepsoi",
          boost::program_options::bool_switch( &(*_boolOptions)[Options::NO_PARALLEL_DEEPSOI] )->default_value( (*_boolOptions)[Options::NO_PARALLEL_DEEPSOI] ),
          "Do not use the parallel deep-soi solving mode when multiple threads are allowed." )
#ifdef ENABLE_GUROBI
        ( "lp-solver",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::LP_SOLVER]) )->default_value( (*_stringOptions)[Options::LP_SOLVER] ),
          "Solver for the LPs during the complete analysis: native/gurobi." )
        ( "num-simulations",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUMBER_OF_SIMULATIONS]) )->default_value( (*_intOptions)[Options::NUMBER_OF_SIMULATIONS] ),
          "Number of simulations generated per neuron." )
        ( "lp-tightening-after-split",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::PERFORM_LP_TIGHTENING_AFTER_SPLIT]) )->default_value( (*_boolOptions)[Options::PERFORM_LP_TIGHTENING_AFTER_SPLIT] ),
          "Whether to skip a LP tightening after a case split." )
        ( "milp-timeout",
          boost::program_options::value<float>( &((*_floatOptions)[Options::MILP_SOLVER_TIMEOUT]) )->default_value( (*_floatOptions)[Options::MILP_SOLVER_TIMEOUT] ),
          "Per-ReLU timeout for iterative propagation." )
        ( "milp-tightening",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE ]) )->default_value((*_stringOptions)[Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE ]) ,
          "The MILP solver bound tightening type: lp/lp-inc/milp/milp-inc/iter-prop/none." )
#endif
        ;

    _optionDescription.add( _positional ).add( _common ).add( _other ).add( _expert );

    // Positional options, for the mandatory options
    _positionalOptions.add( "input", 1 );
    _positionalOptions.add( "property", 2 );
}

void OptionParser::parse( int argc, char **argv )
{
    boost::program_options::store
        ( boost::program_options::command_line_parser( argc, argv )
          .options( _optionDescription ).positional( _positionalOptions ).run(),
          _variableMap );
    boost::program_options::notify( _variableMap );
}

bool OptionParser::valueExists( const String &option )
{
    return _variableMap.count( option.ascii() ) != 0;
}

int OptionParser::extractIntValue( const String &option )
{
    ASSERT( valueExists( option ) );
    return _variableMap[option.ascii()].as<int>();
}

void OptionParser::printHelpMessage() const
{
    std::cerr << "\nusage: ./Marabou <network.nnet> <property> [<options>]\n" << std::endl;
    std::cerr <<  "OR     ./Marabou --input-query <input-query-file> [<options>]\n" << std::endl;

    std::cerr << "You might also consider using the ./resources/runMarabou.py "
              << "script, see README.md for more information." << std::endl;

    std::cerr << "\nBelow are the possible options:" << std::endl;

    std::cerr << "\n" << _common << std::endl;
    std::cerr << "\n" << _other << std::endl;
    std::cerr << "\n" << _expert << std::endl;
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
