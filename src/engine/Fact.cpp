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
    : _causedBySplit( false )
{
}

List<unsigned> Fact::getExplanations() const
{
    return _explanations;
}

void Fact::addExplanation( unsigned explanationID )
{
    _explanations.append( explanationID );
}

void Fact::setCausingConstraintAndSplitID( unsigned constraintID, unsigned splitID )
{
    _causedBySplit = true;
    _causingConstraintID = constraintID;
    _causingSplitID = splitID;
}

bool Fact::isCausedBySplit() const
{
    return _causedBySplit;
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
