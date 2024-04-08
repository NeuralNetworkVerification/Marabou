/*********************                                                        */
/*! \file TableauFactory.h
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

#ifndef __T__TableauFactory_h__
#define __T__TableauFactory_h__

#include "cxxtest/Mock.h"

class ITableau;
class IBoundManager;
class ISelector;

namespace T {
ITableau *createTableau( IBoundManager &BoundManager );
void discardTableau( ITableau *tableau );
} // namespace T

CXXTEST_SUPPLY( createTableau,
                ITableau *,
                createTableau,
                ( IBoundManager & boundManager ),
                T::createTableau,
                ( boundManager ) );

CXXTEST_SUPPLY_VOID( discardTableau,
                     discardTableau,
                     ( ITableau * tableau ),
                     T::discardTableau,
                     ( tableau ) );

#endif // __T__TableauFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
