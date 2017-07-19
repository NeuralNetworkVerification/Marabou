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
#include "TableauRow.h"
#include "Queue.h"

class Tightening
{
public:
	/*
	  The variable to tighten.
	*/
	unsigned variable;

	/*
	  Its new value.
	*/
	double value;

	/*
	  Whether the tightening tightens the
	  lower bound or the upper bound.
	*/
	enum BoundType {
		LB = 0,
		UB,
    };
    BoundType type;
    
	/*
	  Tighten this request in the given tableau.
	*/
	bool tighten( ITableau& tableau ) const;
};

class BoundTightener
{
public:
	/*
	  Derive and enqueue new bounds for the given basic variable
	  in the given tableau.
	*/
	void deriveTightenings( ITableau& tableau, unsigned variable );

	/*
	  Add a given tightening to the queue.
	*/
	void enqueueTightening( const Tightening& tightening );

  /*
    Tighten all enqueued requests.
  */
  bool tighten( ITableau& tableau );

private:
	Queue<Tightening> _tighteningRequests;
};

#endif // __BoundTightener_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
