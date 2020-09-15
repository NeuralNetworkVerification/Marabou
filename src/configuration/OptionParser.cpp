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
    : _optionDescription( "Supported options" )
    , _boolOptions( boolOptions )
    , _intOptions( intOptions )
    , _floatOptions( floatOptions )
    , _stringOptions( stringOptions )
{
}

void OptionParser::initialize()
{
    // Possible options
    _optionDescription.add_options()
        ( "pl-aux-eq",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS]) ),
          "PL constraints generate auxiliary equations" )
        ( "dnc",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::DNC_MODE]) ),
          "Use the divide-and-conquer solving mode" )
        ( "restore-tree-states",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::RESTORE_TREE_STATES]) ),
          "Restore tree states in dnc mode" )
        ( "iter-prop",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::ITERATIVE_PROPAGATION]) ),
          "Use iterative propagation" )
        ( "input",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::INPUT_FILE_PATH]) ),
          "Neural netowrk file" )
        ( "property",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::PROPERTY_FILE_PATH]) ),
          "Property file" )
        ( "input-query",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::INPUT_QUERY_FILE_PATH]) ),
          "Input Query file" )
        ( "summary-file",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SUMMARY_FILE]) ),
          "Summary file" )
        ( "query-dump-file",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::QUERY_DUMP_FILE]) ),
          "Query dump file" )
        ( "num-workers",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_WORKERS]) ),
          "(DNC) Number of workers" )
        ( "split-strategy",
          boost::program_options::value<std::string>( &((*_stringOptions)[Options::SNC_SPLITTING_STRATEGY]) ),
          "The splitting strategy" )
        ( "initial-divides",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_INITIAL_DIVIDES]) ),
          "(DNC) Number of times to initially bisect the input region" )
        ( "initial-timeout",
          boost::program_options::value<int>( &((*_intOptions)[Options::INITIAL_TIMEOUT]) ),
          "(DNC) The initial timeout" )
        ( "num-online-divides",
          boost::program_options::value<int>( &((*_intOptions)[Options::NUM_ONLINE_DIVIDES]) ),
          "(DNC) Number of times to further bisect a sub-region when a timeout occurs" )
        ( "timeout",
          boost::program_options::value<int>( &((*_intOptions)[Options::TIMEOUT]) ),
          "Global timeout" )
        ( "verbosity",
          boost::program_options::value<int>( &((*_intOptions)[Options::VERBOSITY]) ),
          "Verbosity of engine::solve(). 0: does not print anything (for DnC), 1: print"
          "out statistics in the beginning and end, 2: print out statistics during solving." )
        ( "split-threshold",
          boost::program_options::value<int>( &((*_intOptions)[Options::CONSTRAINT_VIOLATION_THRESHOLD]) ),
          "Max number of tries to repair a relu before splitting" )
        ( "iter-prop-timeout",
          boost::program_options::value<int>( &((*_intOptions)[Options::ITERATIVE_PROPAGATION_PER_RELU_TIMEOUT]) ),
          "Per-ReLU timeout for iterative propagation" )
        ( "timeout-factor",
          boost::program_options::value<float>( &((*_floatOptions)[Options::TIMEOUT_FACTOR]) ),
          "(DNC) The timeout factor" )
        ( "help",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::HELP]) ),
          "Prints the help message")
        ( "version",
          boost::program_options::bool_switch( &((*_boolOptions)[Options::VERSION]) ),
          "Prints the version number")

        ;

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
    std::cerr << _optionDescription << std::endl;
};


//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
