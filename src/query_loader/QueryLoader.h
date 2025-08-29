/*********************                                                        */
/*! \file QueryLoader.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __QueryLoader_h__
#define __QueryLoader_h__

#include "IQuery.h"

#define QL_LOG( x, ... ) LOG( GlobalConfiguration::QUERY_LOADER_LOGGING, "QueryLoader: %s\n", x )

class QueryLoader
{
public:
    unsigned _numVars;
    unsigned _numEquations;
    unsigned _numConstraunsigneds;

    /*
      Parse a serialized query and return it in Query form
    */
    static void loadQuery( const String &fileName, IQuery &inputQuery );
};

#endif // __QueryLoader_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
