/*********************                                                        */
/*! \file RowBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __RowBoundTightener_h__
#define __RowBoundTightener_h__

#include "IRowBoundTightener.h"
#include "ITableau.h"
#include "Queue.h"
#include "TableauRow.h"
#include "Tightening.h"

class RowBoundTightener : public IRowBoundTightener
{
public:
    RowBoundTightener();
    ~RowBoundTightener();

    /*
      Allocate internal work memory according to the tableau size.
    */
    void initialize( const ITableau &tableau );

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    void reset( const ITableau &talbeau );

    /*
      Callbacks from the Tableau, to inform of bounds tightened by,
      e.g., the PL constraints.
    */
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Derive and enqueue new bounds for all varaibles, using the
      original constraint matrix A and right hands side vector b. Can
      also do this until saturation, meaning that we continue until no
      new bounds are learned.
    */
    void examineConstraintMatrix( const ITableau &tableau, bool untilSaturation );

    /*
      Derive and enqueue new bounds immedaitely following a pivot
      operation in the given tableau. The tightening is performed for
      the entering variable (which is now basic).
    */
    void examinePivotRow( ITableau &tableau );

    /*
      Get the tightenings entailed by the constraint.
    */
    void getRowTightenings( List<Tightening> &tightenings ) const;

    /*
      Have the Bound Tightener start reporting statistics.
     */
    void setStatistics( Statistics *statistics );

private:
    unsigned _n;

    /*
      Work space for the tightener to derive tighter bounds. These
      represent the tightest bounds currently known, either taken
      from the tableau or derived by the tightener. The flags indicate
      whether each bound has been tightened by the tightener.
    */
    double *_lowerBounds;
    double *_upperBounds;
    bool *_tightenedLower;
    bool *_tightenedUpper;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Free internal work memory.
    */
    void freeMemoryIfNeeded();

    /*
      Do a single pass over the constraint matrix and derive any
      tighter bounds. Return true if new bounds are learned, false
      otherwise.
    */
    bool onePassOverConstraintMatrix( const ITableau &tableau );

    /*
      Process the tableau row and attempt to derive tighter
      lower/upper bounds for the specified variable. Return true iff
      a tighter bound has been found.
     */
    bool tightenOnSingleRow( const ITableau &tableau, unsigned row, unsigned varBeingTightened );
};

#endif // __RowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
