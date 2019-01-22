/*********************                                                        */
/*! \file ConstraintBoundTightener.h
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

#ifndef __ConstraintBoundTightener_h__
#define __ConstraintBoundTightener_h__

#include "IConstraintBoundTightener.h"

class ConstraintBoundTightener : public IConstraintBoundTightener
{
public:
    ConstraintBoundTightener( const ITableau &tableau );
    ~ConstraintBoundTightener();

    /*
      Allocate internal work memory according to the tableau size.
    */
    void setDimensions();

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    void resetBounds();

    /*
      Callback from the Tableau, to inform of a change in dimensions
    */
    void notifyDimensionChange( unsigned m, unsigned n );

    /*
      Callbacks from the Tableau, to inform of bounds tightened by,
      e.g., the PL constraints.
    */
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Have the Bound Tightener start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

    /*
      This method can be used by clients to tell the bound tightener
      about a tighter bound
    */
    void registerTighterLowerBound( unsigned variable, double bound );
    void registerTighterUpperBound( unsigned variable, double bound );

    /*
      Get the tightenings previously registered by the constraints
    */
    void getConstraintTightenings( List<Tightening> &tightenings ) const;

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
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Free internal work memory.
    */
    void freeMemoryIfNeeded();
};

#endif // __ConstraintBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
