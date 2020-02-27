/*********************                                                        */
/*! \file Options.cpp
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

#include "ConfigurationError.h"
#include "Debug.h"
#include "Options.h"

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

    /*
      Int options
    */
    _intOptions[NUM_WORKERS] = 4;
    _intOptions[NUM_INITIAL_DIVIDES] = 0;
    _intOptions[NUM_ONLINE_DIVIDES] = 2;
    _intOptions[INITIAL_TIMEOUT] = 5;
    _intOptions[VERBOSITY] = 2;
    _intOptions[TIMEOUT] = 0;
    _intOptions[SPLIT_THRESHOLD] = 20;

    /*
      Float options
    */
    _floatOptions[TIMEOUT_FACTOR] = 1.5;

    /*
      String options
    */
    _stringOptions[INPUT_FILE_PATH] = "";
    _stringOptions[PROPERTY_FILE_PATH] = "";
    _stringOptions[INPUT_QUERY_FILE_PATH] = "";
    _stringOptions[SUMMARY_FILE] = "";
}

void Options::parseOptions( int argc, char **argv )
{
    _optionParser.parse( argc, argv );
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
