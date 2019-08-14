/*********************                                                        */
/*! \file BlandsRule.h
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

#ifndef __BlandsRule_h__
#define __BlandsRule_h__

#include "EntrySelectionStrategy.h"

class BlandsRule : public EntrySelectionStrategy
{
public:
    /*
      Apply Bland's rule: choose the candidate associated with the
      variable that has the smallest lexicographical index.
    */
    bool select( ITableau &tableau,
                 const List<unsigned> &candidates,
                 const Set<unsigned> &excluded );
};

#endif // __BlandsRule_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
