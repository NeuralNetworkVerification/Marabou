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

class Fact
{
public:
    Fact();
    virtual ~Fact(){}
    List<const Fact*> getExplanations() const;
    void addExplanation( const Fact* explanation );
    void setCausingSplitInfo( unsigned constraintID, unsigned splitID, unsigned splitLevelCausing );
    bool isCausedBySplit() const;
    unsigned getSplitLevelCausing() const;
    unsigned getCausingConstraintID() const;
    unsigned getCausingSplitID() const;
    virtual String getDescription() const = 0;
    void dump() const {printf("%s", getDescription().ascii());}

private:
    List<const Fact*> _explanations;
    unsigned _causingConstraintID;
    unsigned _causingSplitID;
    bool _causedBySplit;
    unsigned _splitLevelCausing;
};

#endif // __Fact_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
