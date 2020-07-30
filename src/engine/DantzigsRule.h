/*********************                                                        */
/*! \file DantzigsRule.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __DantzigsRule_h__
#define __DantzigsRule_h__

#include "EntrySelectionStrategy.h"

#define DANTZIG_LOG( x, ... ) LOG( GlobalConfiguration::DANTZIGS_RULE_LOGGING, "DantzigsRule: %s\n", x )

class String;

class DantzigsRule : public EntrySelectionStrategy
{
public:
    /*
      Apply Dantzig's rule: choose the candidate associated with the
      largest coefficient (in absolute value) in the cost function.
    */
    bool select( ITableau &tableau,
                 const List<unsigned> &candidates,
                 const Set<unsigned> &excluded );
};

#endif // __DantzigsRule_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
