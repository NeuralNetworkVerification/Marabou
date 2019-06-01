/*********************                                                        */
/*! \file LargestIntervalDivider.h
** \verbatim
** Top contributors (to current version):
**   Haoze Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __LargestIntervalDivider_h__
#define __LargestIntervalDivider_h__

#include "Debug.h"
#include "Engine.h"
#include "PiecewiseLinearCaseSplit.h"
#include "QueryDivider.h"

#include <math.h>
#include <vector>

class LargestIntervalDivider : public QueryDivider
{
 public:
  LargestIntervalDivider( List<unsigned>& inputVariables,
                          double timeout_factor );

  void createSubQueries( unsigned num_new_subqueries, SubQuery&
                         previousSubquery, SubQueries& subqueries );

  // Returns the variable with the largest range
  unsigned getLargestInterval( InputRegion& inputRegion );

 private:
  std::vector<unsigned> _inputVariables; // All input variables of the network
  double _timeout_factor; // Multiply the previous timeout with this factor

};

#endif // __LargestIntervalDivider_h__
