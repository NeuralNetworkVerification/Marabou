/*********************                                                        */
/*! \file TableauFactory.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Tableau.h"

namespace T {
ITableau *createTableau( IBoundManager &boundManager )
{
    return new Tableau( boundManager );
}

void discardTableau( ITableau *tableau )
{
    delete tableau;
}
} // namespace T

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
