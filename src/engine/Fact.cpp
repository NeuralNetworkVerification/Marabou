/*********************                                                        */
/*! \file Fact.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Fact.h"

Fact::Fact()
    : _causedBySplit( false ),
      _splitLevelCausing( 0 )
{
}

List<const Fact*> Fact::getExplanations() const
{
    return _explanations;
}

void Fact::addExplanation( const Fact* explanation )
{
    if(explanation!=NULL){
      _explanations.append( explanation );
      if ( explanation->getSplitLevelCausing() > _splitLevelCausing )
        _splitLevelCausing = explanation->_splitLevelCausing;
    }
}

void Fact::setCausingSplitInfo( unsigned constraintID, unsigned splitID, unsigned splitLevelCausing )
{
    _causedBySplit = true;
    _causingConstraintID = constraintID;
    _causingSplitID = splitID;
    _splitLevelCausing = splitLevelCausing;
}

bool Fact::isCausedBySplit() const
{
    return _causedBySplit;
}

unsigned Fact::getSplitLevelCausing() const
{
  return _splitLevelCausing;
}

unsigned Fact::getCausingConstraintID() const
{
    return _causingConstraintID;
}

unsigned Fact::getCausingSplitID() const
{
    return _causingSplitID;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
