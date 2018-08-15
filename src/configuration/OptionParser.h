/*********************                                                        */
/*! \file OptionParser.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __OptionParser_h__
#define __OptionParser_h__

#include "MString.h"
#include "Map.h"
#include "boost/program_options.hpp"

class OptionParser
{
public:
    OptionParser();
    OptionParser( Map<unsigned, bool> *boolOptions );

    /*
      Parse the command line arguments and extract the option values.
    */
    void parse( int argc, char **argv );

    /*
      Check whether a given key has a value.
    */
    bool valueExists( const String &option );

    /*
      Extract the value of a given key.
    */
    int extractIntValue( const String &option );

    /*
      Sets the allowed options and their default values
    */
    void initialize();

private:
    boost::program_options::variables_map _variableMap;
    boost::program_options::options_description _optionDescription;

    Map<unsigned, bool> *_boolOptions;
};

#endif // __OptionParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
