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
class Statistics;

class EntrySelectionStrategy
{
public:
    EntrySelectionStrategy();
    virtual ~EntrySelectionStrategy() {};

    /*
      Perform any necessary initialization work for this strategy.
      This is done in the engine after the tableau is set up.
    */
    virtual void initialize( const ITableau & /* tableau */ ) {};

    /*
      Choose the entrying variable for the given tableau.
    */
    virtual bool select( ITableau &tableau ) = 0;

    /*
      This hook gets called after the entering and leaving variables
      have been selected, but before the actual pivot.
    */
    virtual void prePivotHook( const ITableau &/* tableau */, bool /* fakePivot */ ) {};

    /*
      This hook gets called the pivot operation has been performed.
    */
    virtual void postPivotHook( const ITableau &/* tableau */, bool /* fakePivot */ ) {};

    /*
      This hook is called when the tableau has been resized.
    */
    virtual void resizeHook( const ITableau &/* tableau */ ) {};

    /*
      For reporting statistics
    */
    void setStatistics( Statistics *statistics );

protected:
    /*
      Statistics collection
    */
    Statistics *_statistics;
};

#endif // __EntrySelectionStrategy_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
