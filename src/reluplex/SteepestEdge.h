/*********************                                                        */
/*! \file SteepestEdge.h
** \verbatim
** Top contributors (to current version):
**   Rachel Lim
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __SteepestEdge_h__
#define __SteepestEdge_h__

#include "EntrySelectionStrategy.h"

class SteepestEdgeRule : public EntrySelectionStrategy
{
public:
    /*
      Apply steepest edge pivot selection rule: choose the candidate that maximizes the
      magnitude of the gradient of the cost function with respect to the step direction.
    */
    bool select( ITableau &tableau );

private:
    /*
      Helper function to compute gradient of cost function with respect to edge direction.
    */
    double computeGradient( const unsigned j, const double *c, const double *gamma );
};

#endif // __SteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
