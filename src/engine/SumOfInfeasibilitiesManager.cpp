/*********************                                                        */
/*! \file SumOfInfeasibilitiesManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/


#include "SumOfInfeasibilitiesManager.h"

SumOfInfeasibilitiesManager::SumOfInfeasibilitiesManager
( const List<PiecewiseLinearConstraint *> &plConstraints )
    : _plConstraints( plConstraints )
{}
