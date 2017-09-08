/*********************                                                        */
/*! \file NestedDantzigsRule.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __NestedDantzigsRule_h__
#define __NestedDantzigsRule_h__

#include "Set.h"
#include "EntrySelectionStrategy.h"

class NestedDantzigsRule : public EntrySelectionStrategy
{
public:
	/*
      Apply the nested version of Dantzig's rule: use Dantzig's rule,
      but restrict the search candidates to a shrinking set of nonbasics.
    */
    bool select( ITableau &tableau, const Set<unsigned> &excluded );

    /*
      Initialize nesting set to all nonbasic indices.
    */
    void initialize( const ITableau &tableau );

private:
	Set<unsigned> _J;
};

#endif // __NestedDantzigsRule_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
