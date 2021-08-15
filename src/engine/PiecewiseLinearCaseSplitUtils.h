/*********************                                                        */
/*! \file DantzigsRule.h
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

#ifndef PIECEWISELINEARCASESPLITUTILS_H
#define PIECEWISELINEARCASESPLITUTILS_H

#include "PiecewiseLinearCaseSplit.h"

bool isActiveSplit(PiecewiseLinearCaseSplit split);
unsigned int getSplitVariable(PiecewiseLinearCaseSplit currentSplit);
PiecewiseLinearCaseSplit getSplitFromVariable(unsigned int variable_index);
PiecewiseLinearCaseSplit getAlternativeSplit(PiecewiseLinearCaseSplit split);

#endif // PIECEWISELINEARCASESPLITUTILS_H
