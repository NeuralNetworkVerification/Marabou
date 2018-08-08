/*********************                                                        */
/*! \file ActivationSignature.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "MString.h"
#include "InputQuery.h"
#include "ActivationSignature.h"

void ActivationSignature::processInputQuery( const InputQuery &inputQuery )
{
    if ( inputQuery.countInfiniteBounds() != 0 )
    {
        log( "Have variables with infinite bounds, not doing anything" );
        return;
    }

    List<unsigned> inputs = inputQuery.getInputVariables();
    if ( inputs.empty() )
    {
        log( "No variables marked as input variables, not doing anything" );
        return;
    }


}

void ActivationSignature::log( const String &message ) const
{
    printf( "ActivationSignature: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
