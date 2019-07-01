/*********************                                                        */
/*! \file Options.h
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

#ifndef __Options_h__
#define __Options_h__

#include "MString.h"
#include "Map.h"
#include "OptionParser.h"
#include "boost/program_options.hpp"

/*
  A singleton class that contains all the options and their values.
*/
class Options
{
public:
    enum BoolOptions {
        PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS = 0,
        DNC_MODE ,
    };

    enum IntOptions {
        NUM_WORKERS = 0,
        NUM_INITIAL_DIVIDES,
        NUM_ONLINE_DIVIDES,
        INITIAL_TIMEOUT,
        VERBOSITY,
        TIMEOUT,
    };

    enum FloatOptions{
        TIMEOUT_FACTOR,
    };

    enum StringOptions {
        INPUT_FILE_PATH = 0,
        PROPERTY_FILE_PATH,
        SUMMARY_FILE,
    };

    /*
      The singleton instance
    */
    static Options *get();

    /*
      Parse the command line arguments and extract the option values.
    */
    void parseOptions( int argc, char **argv );

    /*
      Retrieve the value of a boolean option
    */
    bool getBool( unsigned option ) const;

    int getInt (unsigned option) const;

    float getFloat (unsigned option) const;

    /*
      Retrieve the value of a string option
    */
    String getString( unsigned option ) const;

private:
    /*
      Disable default constructor and copy constructor
    */
    Options();
    Options( const Options & );

    /*
      Initialize the default option values
    */
    void initializeDefaultValues();

    OptionParser _optionParser;

    /*
      The various option values
    */
    Map<unsigned, bool> _boolOptions;
    Map<unsigned, int> _intOptions;
    Map<unsigned, float> _floatOptions;
    Map<unsigned, std::string> _stringOptions;
};

#endif // __Options_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
