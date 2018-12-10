/*********************                                                        */
/*! \file IRowBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __IRowBoundTightener_h__
#define __IRowBoundTightener_h__

#include "ITableau.h"
#include "Tightening.h"

class IRowBoundTightener : public ITableau::VariableWatcher, public ITableau::ResizeWatcher
{
public:
    enum Saturation {
        ONE_PASS,
        UNTIL_SATURATION,
    };

    virtual ~IRowBoundTightener() {};

    /*
      Allocate internal work memory according to the tableau size.
    */
    virtual void setDimensions() = 0;

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    virtual void resetBounds() = 0;

    /*
      Clear all learned bounds, without reallocating memory.
    */
    virtual void clear() = 0;


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
    virtual void examineInvertedBasisMatrix( const double *invertedBasis, const double *rhs, Saturation saturation ) = 0;
    virtual void extractRowsFromInvertedBasisMatrix( const double *invertedBasis, const double *rhs ) = 0;
    virtual void examineInvertedBasisMatrix( Saturation saturation ) = 0;

    /*
      Process the inverted basis row and attempt to derive tighter
      lower/upper bounds for the specified variable. Return the number
      of tighter bounds found.

      In the "stored" version, we process a row that has been previously
      stored, by its index.
    */
    virtual unsigned tightenOnSingleInvertedBasisRow( const SparseTableauRow &row ) = 0;
    virtual unsigned tightenOnSingleInvertedStoredBasisRow( unsigned rowIndex ) = 0;

    /*
      Derive and enqueue new bounds for all varaibles, implicitly using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Inv(B0) is not computed directly --- instead, the computation
      is performed via FTRANs. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
    */
    virtual void examineImplicitInvertedBasisMatrix( Saturation saturation ) = 0;

    /*
      Derive and enqueue new bounds for all varaibles, using the
      original constraint matrix A and right hands side vector b. Can
      also do this until saturation, meaning that we continue until no
      new bounds are learned.
    */
    virtual void examineConstraintMatrix( Saturation saturation ) = 0;

    /*
      Derive and enqueue new bounds immedaitely following a pivot
      operation in the given tableau. The tightening is performed for
      the entering variable (which is now basic).
    */
    virtual void examinePivotRow() = 0;

    /*
      Get the tightenings entailed by the constraint.
    */
    virtual void getRowTightenings( List<Tightening> &tightenings ) const = 0;

    /*
      Have the Bound Tightener start reporting statistics.
     */
    virtual void setStatistics( Statistics *statistics ) = 0;

    virtual void debug() = 0;
};

#endif // __IRowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
