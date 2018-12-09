/*********************                                                        */
/*! \file PiecewiseLinearCaseSplit.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __PiecewiseLinearCaseSplit_h__
#define __PiecewiseLinearCaseSplit_h__

#include "Equation.h"
#include "IEngine.h"
#include "Pair.h"
#include "Tightening.h"

class PiecewiseLinearCaseSplit
{
public:
    /*
      Store information regarding a bound tightening.
    */
    void storeBoundTightening( const Tightening &tightening );
    List<Tightening> getBoundTightenings() const;

    /*
      Store information regarding a new equation to be added.
    */
    void addEquation( const Equation &equation );
  	List<Equation> getEquations() const;

    /*
      Set contraint and split ID that caused this split
    */
    void setConstraintAndSplitID( unsigned constraintID, unsigned splitID );
    void setFactsConstraintAndSplitID();
    unsigned getConstraintID() const;
    unsigned getSplitID() const;

    void addExplanation( unsigned causeID );

    /*
      Dump the case split - for debugging purposes.
    */
    void dump() const;

    /*
      Equality operator.
    */
    bool operator==( const PiecewiseLinearCaseSplit &other ) const;

private:
    /*
      Bound tightening information.
    */
    List<Tightening> _bounds;

    /*
      The equation that needs to be added.
    */
    List<Equation> _equations;

    /*
      Information about PLConstraint and Split that caused this
    */
    // Guy: I thought we reached the conclusion that this information should be stored elsewhere? E.g., in the fact tracker?
    unsigned _constraintID;
    unsigned _splitID;
};

#endif // __PiecewiseLinearCaseSplit_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
