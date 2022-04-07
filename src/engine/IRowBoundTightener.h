/*********************                                                        */
/*! \file IRowBoundTightener.h
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

#ifndef __IRowBoundTightener_h__
#define __IRowBoundTightener_h__

#include "ITableau.h"
#include "Tightening.h"

class IRowBoundTightener : public ITableau::VariableWatcher, public ITableau::ResizeWatcher
{
public:
    virtual ~IRowBoundTightener() {};

    /*
      Allocate internal work memory according to the tableau size.
    */
    virtual void setDimensions() = 0;

    /*
      Derive and enqueue new bounds for all varaibles, using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
     */
    virtual void examineInvertedBasisMatrix( bool untilSaturation ) = 0;

    /*
      Derive and enqueue new bounds for all varaibles, implicitly using the
      inverse of the explicit basis matrix, inv(B0), which should be available
      through the tableau. Inv(B0) is not computed directly --- instead, the computation
      is performed via FTRANs. Can also do this until saturation, meaning that we
      continue until no new bounds are learned.
    */
    virtual void examineImplicitInvertedBasisMatrix( bool untilSaturation ) = 0;

    /*
      Derive and enqueue new bounds for all varaibles, using the
      original constraint matrix A and right hands side vector b. Can
      also do this until saturation, meaning that we continue until no
      new bounds are learned.
    */
    virtual void examineConstraintMatrix( bool untilSaturation ) = 0;

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

    /*
       Update pointers to local lower/upper bounds in BoundManager
     */
    virtual void setBoundsPointers( const double *lower, const double *upper ) = 0;

};

#endif // __IRowBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
