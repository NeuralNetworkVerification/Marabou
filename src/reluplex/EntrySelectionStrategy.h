/*********************                                                        */
/*! \file EntrySelectionStrategy.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __EntrySelectionStrategy_h__
#define __EntrySelectionStrategy_h__

#include "List.h"

class ITableau;

class EntrySelectionStrategy
{
public:
    virtual ~EntrySelectionStrategy() {};

    /*
      Choose the entrying variable from the list of eligible-for-entry
      candidates. The candidates are given using the internal indices
      of the tableau, i.e. are in the range [1,n-m].
    */
    virtual unsigned select( const List<unsigned> &candidates, const ITableau &tableau ) = 0;
};

#endif // __EntrySelectionStrategy_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
