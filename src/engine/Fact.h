/*********************                                                        */
/*! \file Fact.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Fact_h__
#define __Fact_h__

#include "List.h"
#include "Map.h"
#include "MString.h"

class FactTracker;

class Fact
{
public:
    Fact();
    virtual ~Fact(){}

    /*
      Add/get explanations of a learned fact
    */
    List<const Fact*> getExplanations() const;
    void addExplanation( const Fact* explanation );

    /*
      Set/get information about the split that caused this fact, if there are no explanations for it
    */
    void setCausingSplitInfo( unsigned constraintID, unsigned splitID, unsigned splitLevelCausing );
    bool isCausedBySplit() const;
    unsigned getSplitLevelCausing() const;
    unsigned getCausingConstraintID() const;
    unsigned getCausingSplitID() const;

    /*
      Get description of fact
    */
    virtual String getDescription() const = 0;
    void dump() const {printf("%s", getDescription().ascii());}

    /*
      Check whether fact represents an equation
    */
    virtual bool isEquation() const = 0;

    /*
      Pointer to the owner, i.e. FactTracker object responsible for managing this fact
    */
    void setOwner(const FactTracker* owner);
    FactTracker* getOwner() const;

private:

    // List of explanations from which this fact can be deduced
    List<const Fact*> _explanations;

    // If this fact was not explainable and due to a split, information about the split that caused it
    unsigned _causingConstraintID;
    unsigned _causingSplitID;
    bool _causedBySplit;
    unsigned _splitLevelCausing;

    // FactTracker responsible for managing this fact
    FactTracker* _owner;
};

#endif // __Fact_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
