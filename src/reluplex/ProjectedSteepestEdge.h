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
      Allocate and initialize data structures according to the size of the tableau.
    */
    void initialize( const ITableau &tableau );

    /*
      Apply the projected steepest edge pivot selection rule.
    */
    bool select( ITableau &tableau );

    /*
      Use this hook to update gamma according to the entering and leaving
      variables.
    */
    void prePivotHook( const ITableau &tableau );

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

    /*
      Reset the reference space and gamma, according to the current non-basic variables.
    */
    void resetReferenceSpace( const ITableau &tableau );

    /*
      Compute the accurate value of gamma for the given index, and measure the error
      when compared to the approximate gamma.
    */
    double computeAccurateGamma( double &accurateGamma, const ITableau &tableau );
};

#endif // __ProjectedSteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
