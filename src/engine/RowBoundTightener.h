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

#include "Equation.h"
#include "IRowBoundTightener.h"
#include "ITableau.h"
#include "Queue.h"
#include "TableauRow.h"
#include "Tightening.h"

class RowBoundTightener : public IRowBoundTightener
{
public:
    RowBoundTightener( const ITableau &tableau );
    ~RowBoundTightener();

    /*
      Allocate internal work memory according to the tableau size
    */
    void setDimensions();

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    void resetBounds();

    /*
      Clear all learned bounds, without reallocating memory.
    */
    void clear();

    /*
      Callbacks from the Tableau, to inform of bounds tightened by,
      e.g., the PL constraints.
    */
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Callback from the Tableau, to inform of a change in dimensions
    */
    void notifyDimensionChange( unsigned m, unsigned n );

    /*
      Derive and enqueue new bounds for all varaibles, using the supplied
      inverse of the explicit basis matrix and the supplied rhs scalars.
      Can also do this until saturation, meaning that we continue until
      no new bounds are learned.

      The procedure has two steps:

      1. Extract rows from the inverted basis matrix
      2. Use the rows for bound tightening

      It is possible to invoke the individual steps in order to save time, e.g.,
      if the same basis matrix is used multiple times.
     */
    void examineInvertedBasisMatrix( const double *invertedBasis, const double *rhs, Saturation saturation );
    void extractRowsFromInvertedBasisMatrix( const double *invertedBasis, const double *rhs );
    void examineInvertedBasisMatrix( Saturation saturation );

    /*
      Derive and enqueue new bounds for all varaibles, implicitly using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Inv(B0) is not computed directly --- instead, the computation
      is performed via FTRANs. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
     */
    void examineImplicitInvertedBasisMatrix( Saturation saturation );

    /*
      Derive and enqueue new bounds for all varaibles, using the
      original constraint matrix A and right hands side vector b. Can
      also do this until saturation, meaning that we continue until no
      new bounds are learned.
    */
    void examineConstraintMatrix( Saturation saturation );

    /*
      Derive and enqueue new bounds immedaitely following a pivot
      operation in the given tableau. The tightening is performed for
      the entering variable (which is now basic).
    */
    void examinePivotRow();

    /*
      Get the tightenings entailed by the constraint.
    */
    void getRowTightenings( List<Tightening> &tightenings ) const;

    /*
      Have the Bound Tightener start reporting statistics.
     */
    void setStatistics( Statistics *statistics );

private:
    const ITableau &_tableau;
    unsigned _n;
    unsigned _m;

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
      Work space for the inverted basis matrix tighteners
    */
    TableauRow **_rows;
    double *_z;
    double *_ciTimesLb;
    double *_ciTimesUb;
    char *_ciSign;

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
      tighter bounds. Return the number of new bounds learned.
    */
    unsigned onePassOverConstraintMatrix();

    /*
      Process the tableau row and attempt to derive tighter
      lower/upper bounds for the specified variable. Return the number of
      tighter bounds found.
     */
    unsigned tightenOnSingleConstraintRow( unsigned row );

    /*
      Do a single pass over the inverted basis rows and derive any
      tighter bounds. Return the number of new bounds learned.
    */
    unsigned onePassOverInvertedBasisRows();

    /*
      Process the inverted basis row and attempt to derive tighter
      lower/upper bounds for the specified variable. Return the number
      of tighter bounds found.
    */
    unsigned tightenOnSingleInvertedBasisRow( const TableauRow &row );
};

#endif // __RowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
