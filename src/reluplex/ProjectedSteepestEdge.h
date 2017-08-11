/*********************                                                        */
/*! \file ProjectedSteepestEdge.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ProjectedSteepestEdge_h__
#define __ProjectedSteepestEdge_h__

#include "EntrySelectionStrategy.h"

class ProjectedSteepestEdgeRule : public EntrySelectionStrategy
{
public:
    ProjectedSteepestEdgeRule();
    ~ProjectedSteepestEdgeRule();

    /*
      Allocate data structures according to the size of the tableau.
    */
    void initialize( const ITableau &tableau );

    /*
      Apply the projected steepest edge pivot selection rule.
    */
    bool select( ITableau &tableau );

private:
    /*
      Indicates whether a variable, basic or non basic, is in the reference space.
    */
    bool *_referenceSpace;

    /*
      The steepest edge gamma function.
    */
    double *_gamma;

    /*
      Work space.
    */
    double *_work;

    /*
      Tableau dimensions.
    */
    unsigned _m;
    unsigned _n;

    void resetReferenceSpace( const ITableau &tableau );


};

#endif // __ProjectedSteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
