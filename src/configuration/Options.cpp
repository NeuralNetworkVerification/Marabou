/*********************                                                        */
/*! \file Options.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
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
    : _optionParser( &_boolOptions, &_stringOptions )
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
      Int options
    */
    _boolOptions[PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS] = false;

    /*
      String options
    */
    _stringOptions[INPUT_FILE_PATH] = "";
    _stringOptions[PROPERTY_FILE_PATH] = "";
}

void Options::parseOptions( int argc, char **argv )
{
    _optionParser.parse( argc, argv );
}

bool Options::getBool( unsigned option ) const
{
    return _boolOptions.get( option );
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
