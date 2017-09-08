/*********************                                                        */
/*! \file IProjectedSteepestEdge.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __IProjectedSteepestEdge_h__
#define __IProjectedSteepestEdge_h__

#include "EntrySelectionStrategy.h"
#include "Set.h"

class String;
class ITableau;

class IProjectedSteepestEdgeRule : public EntrySelectionStrategy
{
public:
    virtual ~IProjectedSteepestEdgeRule() {};

    /*
      Allocate and initialize data structures according to the size of the tableau.
    */
    virtual void initialize( const ITableau &tableau ) = 0;

    /*
      Apply the projected steepest edge pivot selection rule.
    */
    virtual bool select( ITableau &tableau, const Set<unsigned> &excluded ) = 0;

    /*
      We use this hook to update gamma according to the entering
      and leaving variables.
    */
    virtual void prePivotHook( const ITableau &tableau, bool fakePivot ) = 0;

    /*
      We use this hook to reset the reference space if needed.
    */
    virtual void postPivotHook( const ITableau &tableau, bool fakePivot ) = 0;

    /*
      This hook is called when the tableau has been resized.
    */
    virtual void resizeHook( const ITableau &tableau ) = 0;

    /*
      For debugging purposes.
    */
    virtual double getGamma( unsigned index ) const = 0;
};

#endif // __IProjectedSteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
