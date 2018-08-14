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

private:
    boost::program_options::variables_map _variableMap;
};

#endif // __OptionParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
