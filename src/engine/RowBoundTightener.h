/*********************                                                        */
/*! \file RowBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __RowBoundTightener_h__
#define __RowBoundTightener_h__

#include "IBoundManager.h"
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
       Method obtains lower bound of *var*.
     */
    inline double getLowerBound( unsigned var ) const
    {
        return  _lowerBounds[var];// _boundManager.getLowerBound( var );
    }

    /*
       Method obtains upper bound of *var*.
     */
    inline double getUpperBound( unsigned var ) const
    {
        return _upperBounds[var];//_boundManager.getUpperBound( var );
    }

    /*
     * Register a new tighter bound
     */
    inline unsigned registerTighterLowerBound( unsigned variable, double newLowerBound)
    {
        return _boundManager.tightenLowerBound( variable, newLowerBound ) ? 1u : 0u;
    }

    inline unsigned registerTighterUpperBound( unsigned variable, double newLowerBound)
    {
        return _boundManager.tightenUpperBound( variable, newLowerBound ) ? 1u : 0u;
    }

    /*
      Derive and enqueue new bounds for all varaibles, using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
     */
    void examineInvertedBasisMatrix( bool untilSaturation );

    /*
      Derive and enqueue new bounds for all varaibles, implicitly using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Inv(B0) is not computed directly --- instead, the computation
      is performed via FTRANs. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
     */
    void examineImplicitInvertedBasisMatrix( bool untilSaturation );

    /*
      Derive and enqueue new bounds for all varaibles, using the
      original constraint matrix A and right hands side vector b. Can
      also do this until saturation, meaning that we continue until no
      new bounds are learned.
    */
    void examineConstraintMatrix( bool untilSaturation );

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

    /*
       Update pointers to local lower/upper bounds in BoundManager
     */
    void setBoundsPointers( const double *lower, const double *upper );

private:
    const ITableau &_tableau;
    unsigned _n;
    unsigned _m;

    /*
     * Object that stores current bounds from all the sources
     */
    IBoundManager &_boundManager;

    /*
       Direct pointers to _boundManager arrays to avoid multiple dereferencing.
     */
    const double * _lowerBounds;
    const double * _upperBounds;


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
