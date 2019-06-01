/*********************                                                        */
/*! \file QueryDivider.h
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

#ifndef __QueryDivider_h__
#define __QueryDivider_h__


#include "SubQuery.h"

#include <map>

struct InputRegion {
  std::map<unsigned, double> lowerbounds;
  std::map<unsigned, double> upperbounds;
};


class QueryDivider
{
 public:
  // Divide the previousSubquery into |numNewSubQueries| new subqueries and
  // store them in subqueries
  virtual void createSubQueries( unsigned numNewSubQueries, SubQuery&
                                 previousSubQuery, SubQueries& subqueries ) = 0;

  // Bisect the given input region at the given dimension, and store the
  // new input regions into inputRegions
  void bisectInputRegion(InputRegion& inputRegion, unsigned dimensionToBisect,
                         std::vector<InputRegion>& inputRegions);
};

#endif // __Querydivider_h__
