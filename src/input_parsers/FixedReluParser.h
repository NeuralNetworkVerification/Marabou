/*********************                                                        */
/*! \file FixedReluParser.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __FixedReluParser_h__
#define __FixedReluParser_h__

#include "MString.h"

class FixedReluParser
{
public:
    static void parse( const String &propertyFilePath,
                       Map<unsigned, unsigned> &fixedRelus );
};

#endif // __PropertyParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
