/*********************                                                        */
/*! \file BlandsRule.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BlandsRule.h"
#include "ReluplexError.h"

unsigned BlandsRule::select( const List<unsigned> &candidates )
{
    if ( candidates.empty() )
        throw ReluplexError( ReluplexError::NO_AVAILABLE_CANDIDATES, "BlandsRule" );

    List<unsigned>::const_iterator it = candidates.begin();
    unsigned min = *it;
    ++it;

    while ( it != candidates.end() )
    {
        if ( *it < min )
            min = *it;
        ++it;
    }

    return min;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
