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
#include "Options.h"
#include "OptionParser.h"

OptionParser::OptionParser()
    : _optionDescription( "Supported options" )
{
}

void OptionParser::setOptions()
{
    Options *options = Options::get();
    if (_variableMap.count("pl-aux-eq"))
        options->setBool(Options::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS, _variableMap["pl-aux-eq"].as<bool>());
    if (_variableMap.count("dnc"))
        options->setBool(Options::DNC_MODE, _variableMap["dnc"].as<bool>());
    if (_variableMap.count("input"))
        options->setString(Options::INPUT_FILE_PATH, _variableMap["input"].as<std::string>());
    if (_variableMap.count("property"))
        options->setString(Options::PROPERTY_FILE_PATH, _variableMap["property"].as<std::string>());
    if (_variableMap.count("summary-file"))
        options->setString(Options::SUMMARY_FILE, _variableMap["summary-file"].as<std::string>());
    if (_variableMap.count("num-workers"))
        options->setInt(Options::NUM_WORKERS, _variableMap["num-workers"].as<int>());
    if (_variableMap.count("initial-divides"))
        options->setInt(Options::NUM_INITIAL_DIVIDES, _variableMap["initial-divides"].as<int>());
    if (_variableMap.count("initial-timeout"))
        options->setInt(Options::INITIAL_TIMEOUT, _variableMap["initial-timeout"].as<int>());
    if (_variableMap.count("num-online-divides"))
        options->setInt(Options::NUM_ONLINE_DIVIDES, _variableMap["num-online-divides"].as<int>());
    if (_variableMap.count("timeout"))
        options->setInt(Options::TIMEOUT, _variableMap["timeout"].as<int>());
    if (_variableMap.count("verbosity"))
        options->setInt(Options::VERBOSITY, _variableMap["verbosity"].as<int>());
    if (_variableMap.count("timeout-factor"))
        options->setFloat(Options::TIMEOUT_FACTOR, _variableMap["timeout-factor"].as<float>());
    if (_variableMap.count("help"))
        options->setBool(Options::HELP, _variableMap["help"].as<bool>());
    if (_variableMap.count("version"))
        options->setBool(Options::VERSION, _variableMap["version"].as<bool>());

}

void OptionParser::parse( int argc, char **argv )
{
    // Possible options
    _optionDescription.add_options()
        ( "pl-aux-eq",
          boost::program_options::bool_switch(),
          "PL constraints generate auxiliary equations" )
        ( "dnc",
          boost::program_options::bool_switch(),
          "Use the divide-and-conquer solving mode" )
        ( "input",
          boost::program_options::value<std::string>(),
          "Neural netowrk file" )
        ( "property",
          boost::program_options::value<std::string>(),
          "Property file" )
        ( "summary-file",
          boost::program_options::value<std::string>(),
          "Summary file" )
        ( "num-workers",
          boost::program_options::value<int>(),
          "(DNC) Number of workers" )
        ( "initial-divides",
          boost::program_options::value<int>(),
          "(DNC) Number of times to initially bisect the input region" )
        ( "initial-timeout",
          boost::program_options::value<int>(),
          "(DNC) The initial timeout" )
        ( "num-online-divides",
          boost::program_options::value<int>(),
          "(DNC) Number of times to further bisect a sub-region when a timeout occurs" )
        ( "timeout",
          boost::program_options::value<int>(),
          "Global timeout" )
        ( "verbosity",
          boost::program_options::value<int>(),
          "Verbosity of engine::solve(). 0: does not print anything (for DnC), 1: print"
          "out statistics in the beginning and end, 2: print out statistics during solving." )
        ( "timeout-factor",
          boost::program_options::value<float>(),
          "(DNC) The timeout factor" )
        ( "help",
          boost::program_options::bool_switch(),
          "Prints the help message")
        ( "version",
          boost::program_options::bool_switch(),
          "Prints the version number")

        ;
    // Positional options, for the mandatory options
    _positionalOptions.add( "input", 1 );
    _positionalOptions.add( "property", 2 );
    boost::program_options::store
        ( boost::program_options::command_line_parser( argc, argv )
          .options( _optionDescription ).positional( _positionalOptions ).run(),
          _variableMap );
    boost::program_options::notify( _variableMap );
    setOptions();
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
