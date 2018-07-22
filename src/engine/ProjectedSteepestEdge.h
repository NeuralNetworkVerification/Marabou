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

#include "IProjectedSteepestEdge.h"
#include "SparseVector.h"

class ProjectedSteepestEdgeRule : public IProjectedSteepestEdgeRule
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
    bool select( ITableau &tableau, const Set<unsigned> &excluded );

    /*
      We use this hook to update gamma according to the entering
      and leaving variables.
    */
    void prePivotHook( const ITableau &tableau, bool fakePivot );

    /*
      We use this hook to reset the reference space if needed.
    */
    void postPivotHook( const ITableau &tableau, bool fakePivot );

    /*
      This hook is called when the tableau has been resized.
    */
    void resizeHook( const ITableau &tableau );

    /*
      For debugging purposes.
    */
    double getGamma( unsigned index ) const;

private:
    /*
      Indicates whether a variable, basic or non basic, is in the reference space.
    */
    char *_referenceSpace;

    /*
      The steepest edge gamma function.
    */
    double *_gamma;

    /*
      Work space.
    */
    double *_work1;
    double *_work2;
    SparseVector _AColumn;

    /*
      Tableau dimensions.
    */
    unsigned _m;
    unsigned _n;

    /*
      Remaining iterations before resetting the reference space.
    */
    int _iterationsUntilReset;

    /*
      The error in gamma compuated in the previous iteration.
    */
    double _errorInGamma;

    /*
      Reset the reference space and gamma, according to the current non-basic variables.
    */
    void resetReferenceSpace( const ITableau &tableau );

    /*
      Compute the accurate value of gamma for the given index, and measure the error
      when compared to the approximate gamma.
    */
    double computeAccurateGamma( double &accurateGamma, const ITableau &tableau );

    /*
      Free all data structures.
    */
    void freeIfNeeded();

    static void log( const String &message );
};

#endif // __ProjectedSteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
