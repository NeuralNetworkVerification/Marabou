/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "AcasParser.h"
#include "File.h"
#include "Marabou.h"
#include "Options.h"
#include "PropertyParser.h"
#include "ReluplexError.h"

Marabou::Marabou()
    : _acasParser( NULL )
{
}

Marabou::~Marabou()
{
    if ( _acasParser )
    {
        delete _acasParser;
        _acasParser = NULL;
    }
}

void Marabou::run( int argc, char **argv )
{
    Options *options = Options::get();
    options->parseOptions( argc, argv );

    prepareInputQuery();
    solveQuery();
    displayResults();
}

void Marabou::prepareInputQuery()
{
    /*
      Step 1: extract the network
    */
    String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
    if ( !File::exists( networkFilePath ) )
    {
        printf( "Error: the specified network file (%s) doesn't exist!\n", networkFilePath.ascii() );
        throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
    }

    // For now, assume the network is given in ACAS format
    _acasParser = new AcasParser( networkFilePath );
    _acasParser->generateQuery( _inputQuery );

    /*
      Step 2: extract the property in question
    */
    String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
    if ( propertyFilePath != "" )
        PropertyParser().parse( propertyFilePath, _inputQuery );
}

void Marabou::solveQuery()
{
    _result = _engine.processInputQuery( _inputQuery ) && _engine.solve();
}

void Marabou::displayResults() const
{
    if ( !_result )
    {
        printf( "UNSAT\n" );
        return;
    }

    printf( "SAT\n" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
