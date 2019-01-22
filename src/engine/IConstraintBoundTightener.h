/*********************                                                        */
/*! \file IConstraintBoundTightener.h
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

#ifndef __IConstraintBoundTightener_h__
#define __IConstraintBoundTightener_h__

#include "ITableau.h"
#include "Tightening.h"

class IConstraintBoundTightener : public ITableau::VariableWatcher, public ITableau::ResizeWatcher
{
public:
    virtual ~IConstraintBoundTightener() {};

    /*
      Allocate internal work memory according to the tableau size.
    */
    virtual void setDimensions() = 0;

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    virtual void resetBounds() = 0;

    /*
      Callback from the Tableau, to inform of a change in dimensions
    */
    virtual void notifyDimensionChange( unsigned m, unsigned n ) = 0;

    /*
      Callbacks from the Tableau, to inform of bounds tightened by,
      e.g., the PL constraints.
    */
    virtual void notifyLowerBound( unsigned variable, double bound ) = 0;
    virtual void notifyUpperBound( unsigned variable, double bound ) = 0;

    /*
      Have the Bound Tightener start reporting statistics.
    */
    virtual void setStatistics( Statistics *statistics ) = 0;

    /*
      This method can be used by clients to tell the bound tightener
      about a tighter bound
    */
    virtual void registerTighterLowerBound( unsigned variable, double bound ) = 0;
    virtual void registerTighterUpperBound( unsigned variable, double bound ) = 0;

    /*
      Get the tightenings previously registered by the constraints
    */
    virtual void getConstraintTightenings( List<Tightening> &tightenings ) const = 0;
};

#endif // __IConstraintBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
