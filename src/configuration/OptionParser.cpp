/*********************                                                        */
/*! \file OptionParser.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "Debug.h"
#include "OptionParser.h"

void OptionParser::parse( int argc, char **argv )
{
    // Declare the supported options.
    boost::program_options::options_description optionDescription( "Allowed options" );
    optionDescription.add_options()
        ( "test", "bla" )
        ( "dummy3", boost::program_options::value<int>(), "dummy4")
        ;

    boost::program_options::store
        ( boost::program_options::parse_command_line( argc, argv, optionDescription ), _variableMap );
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
