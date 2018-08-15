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

#include "Marabou.h"
#include "Options.h"

void Marabou::run( int argc, char **argv )
{
    Options *options = Options::get();
    options->parseOptions( argc, argv );

    printf( "Value of aux flag: %u\n", options->getBool( Options::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS ) );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
