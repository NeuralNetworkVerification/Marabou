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
      Initialize tightest lower/upper bounds using the tableau.
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
       Method obtains lower bound of *var*.
     */
    inline double getLowerBound( unsigned var ) const
    {
        return ( _useBoundManager ) ? _boundManager.getLowerBound( var )
                                    : _lowerBounds[var];
    }

    /*
       Method obtains upper bound of *var*.
     */
    inline double getUpperBound( unsigned var ) const
    {
        return ( _useBoundManager ) ? _boundManager.getUpperBound( var )
                                    : _upperBounds[var];
    }

    /*
       Method sets the lower bound of *var* to *value*.
     */
    inline void setLowerBound( unsigned var, double value )
    {
        if ( _useBoundManager )
            _boundManager.setLowerBound( var, value );
        else
            _lowerBounds[var] = value;
    }

    /*
       Method sets the upper bound of *var* to *value*.
     */
    inline void setUpperBound( unsigned var, double value )
    {
        if ( _useBoundManager )
            _boundManager.setUpperBound( var, value );
        else
            _upperBounds[var] = value;
    }

    /*
     * Local register new bound functions
     * NOTE: Used post integration, currently these methods are not used anywhere atm.
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
      Callback from the Tableau, to inform of a change in dimensions
    */
    void notifyDimensionChange( unsigned m, unsigned n );

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
       Enable using the BoundManager to manage bounds.
     */
    void enableBoundManager() { _useBoundManager = true; }

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
     * Object that stores current bounds from all the sources
     */
    IBoundManager &_boundManager;

    /*
      Flag to enable using BoundManager to manager bounds. Temporary and only
      for purposes of integration.
     */
    bool _useBoundManager;

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
