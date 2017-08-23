/*********************                                                        */
/*! \file BoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __BoundTightener_h__
#define __BoundTightener_h__

#include "ITableau.h"
#include "Queue.h"
#include "TableauRow.h"
#include "Tightening.h"

class BoundTightener
{
public:
    BoundTightener();

    /*
      Derive and enqueue new bounds for the given basic variable
      in the given tableau.
    */
    void deriveTightenings( ITableau &tableau );

    /*
      Tighten all enqueued requests.
    */
    void tighten( ITableau &tableau );

    /*
      Have the Bound Tightener start reporting statistics.
     */
    void setStatistics( Statistics *statistics );

    /*
      Clear any pending bound tightenings.
    */
    void clearStoredTightenings();

private:
  	/*
      Add a given tightening to the queue.
    */
    void enqueueTightening( const Tightening &tightening );

    Queue<Tightening> _tighteningRequests;

    /*
      Statistics collection
    */
    Statistics *_statistics;
};

#endif // __BoundTightener_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
